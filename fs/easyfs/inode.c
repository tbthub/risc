#include "dev/blk/blk_dev.h"
#include "easyfs.h"
#include "lib/atomic.h"
#include "lib/math.h"
#include "mm/mm.h"
#include "mm/slab.h"
#include "std/string.h"

struct easy_m_inode root_m_inode;

// 根据 inode 计算在磁盘 inode_area 的 offset
static inline int offset_ino(int ino)
{
    return ino * m_esb.s_ds.inode_size;
}

// 分配 ino 的 inode，是新的
// 搭配 efs_i_alloc 后 fill
static struct easy_m_inode *efs_i_alloc()
{
    struct easy_m_inode *m_inode = kmem_cache_alloc(&efs_inode_kmem_cache);
    if (!m_inode)
        panic("efs_ialloc.\n");

    m_inode->i_flags = 0;
    spin_init(&m_inode->i_lock, "i_lock");
    sleep_init(&m_inode->i_slock, "i_slock");
    atomic_set(&m_inode->i_refcnt, 0);
    m_inode->i_sb = &m_esb;
    INIT_HASH_NODE(&m_inode->i_hnode);
    INIT_LIST_HEAD(&m_inode->i_list);
    INIT_LIST_HEAD(&m_inode->i_dirty);
    m_inode->i_indir = NULL;
    return m_inode;
}

// spin lock

static void efs_i_sdirty(struct easy_m_inode *inode)
{
    SET_FLAG(&inode->i_flags, I_DIRTY);
    if (list_empty(&inode->i_dirty))
        list_add_head(&inode->i_dirty, &inode->i_sb->s_idirty_list);
}

void efs_i_cdirty(struct easy_m_inode *inode)
{
    CLEAR_FLAG(&inode->i_flags, I_DIRTY);
    list_del_init(&inode->i_dirty);
}

// 把 inode 加到超级块链表中,需要加锁
static void efs_i_addsb(struct easy_m_inode *m_inode)
{
    hash_add_head(&m_esb.s_ihash, m_inode->i_di.i_no, &m_inode->i_hnode);
    list_add_head(&m_inode->i_list, &m_esb.s_ilist);
}

//  sleep lock

// 根据 ino 读出磁盘的 d_inode 填充到 m_inode.i_di
static int efs_i_fill(struct easy_m_inode *m_inode, int ino)
{
    assert(ino > 0, "efs_i_fill\n");
    int offset = offset_ino(ino);
    return blk_read(m_inode->i_sb->s_bd, m_esb.s_ds.inode_area_start, offset, sizeof(m_inode->i_di), &m_inode->i_di);
}

// 把 inode 磁盘部分写回
int efs_i_update(struct easy_m_inode *i)
{
    assert(i->i_di.i_no > 0, "efs_i_update i->i_di.i_no\n");
    int offset = offset_ino(i->i_di.i_no);
    // 写回 inode 元数据
    blk_write(i->i_sb->s_bd, m_esb.s_ds.inode_area_start, offset, sizeof(i->i_di), &i->i_di);
    // 如果间接块被读入，则写回间接块
    if (i->i_indir) {
        blk_write_count(i->i_sb->s_bd, i->i_di.i_addrs[NDIRECT], 1, i->i_indir);
        // TODO 这个页面也暂时常驻内存，后面如果让 LRU 回收的时候再释放
        // __free_page(i->i_indir);
    }
    return 0;
}

// static inline void efs_i_free(struct easy_m_inode *i)
// {
//     kmem_cache_free(&efs_inode_kmem_cache, i);
// }

// // 引用计数为 0 后释放，i_put 确保了单线程释放
// // 这个函数可能会睡眠
// // TODO 考虑需不需要在 put 为 0 时候从哈希表中移除
// // TODO 要不要再添加一个 LRU2 来进行管理，不要直接从哈希表中移除
// static void efs_i_del(struct easy_m_inode *i)
// {
//     if (!i)
//         return;

//     spin_lock(&m_esb.s_lock);

//     list_del(&i->i_list);
//     hash_del_node(&m_esb.s_ihash, &i->i_hnode);

//     spin_unlock(&m_esb.s_lock);

//     if (i->i_indir)
//     {
//         blk_write_count(i->i_sb->s_bd, i->i_di.i_addrs[NDIRECT], 1, i->i_indir);
//         __free_page(i->i_indir);
//     }
// }

// 将逻辑块号对应到实际的物理块
// 并在当没有分配块号时为其分配
// 这个函数需要加睡眠锁互斥
static int efs_i_bmap(struct easy_m_inode *inode, int lbno, int alloc)
{
    int bno = 0;
    if (lbno < 0)
        panic("efs_i_bmap");
    // 直接块
    if (lbno < NDIRECT) {
        if ((bno = inode->i_di.i_addrs[lbno]) == 0) {
            if (!alloc)
                return 0;
            spin_lock(&m_esb.s_lock);
            bno = efs_bmap_alloc();
            spin_unlock(&m_esb.s_lock);
            inode->i_di.i_addrs[lbno] = bno;
        }
        return bno;
    }
    // 间接块
    lbno -= NDIRECT;

    if (lbno < NINDIRECT) {
        if ((bno = inode->i_di.i_addrs[NDIRECT]) == 0) {
            if (!alloc)
                return 0;
            spin_lock(&m_esb.s_lock);
            bno = efs_bmap_alloc();
            spin_unlock(&m_esb.s_lock);
            inode->i_di.i_addrs[NDIRECT] = bno;
        }
        if (!inode->i_indir) {
            inode->i_indir = __alloc_page(0);
            blk_read_count(inode->i_sb->s_bd, bno, 1, inode->i_indir);
        }
        uint32_t *blk = inode->i_indir;
        if (blk[lbno] == 0) {
            if (!alloc)
                return 0;
            spin_lock(&m_esb.s_lock);
            blk[lbno] = efs_bmap_alloc();
            spin_unlock(&m_esb.s_lock);
            // 我们选择在最后释放 Inode 的时候，也就是 efs_i_del 统一把这个页面写回
            // 多次睡眠锁成本可能很高，主要成本都在睡眠锁，而不是复制
            // blk_write(inode->i_sb->s_bd, bno, lbno * sizeof(blk[lbno]), sizeof(blk[lbno]), blk + lbno);
        }
        bno = blk[lbno];
    }
    return bno;
}

// 释放 inode
void efs_i_put(struct easy_m_inode *m_inode)
{
    assert(atomic_read(&m_inode->i_refcnt) > 0, "efs_i_put");
    if (atomic_dec_and_test(&m_inode->i_refcnt)) {
        // efs_i_free(m_inode);
    }
}

void efs_i_dup(struct easy_m_inode *inode)
{
    atomic_inc(&inode->i_refcnt);
}

// 获得 ino 索引节点（这个函数会陷入睡眠）
// 这里的 ino 应该是确保有效的，并不是新创建的
struct easy_m_inode *efs_i_get(int ino)
{
    struct easy_m_inode *m_inode = NULL;
    int creating = 0;
    spin_lock(&m_esb.s_lock);
    hash_find(m_inode, &m_esb.s_ihash, i_di.i_no, ino, i_hnode);  // 先在哈希表中查找
    // 如果没有，创建一个新的并加入
    if (!m_inode) {
        // printk("1\n");
        m_inode = efs_i_alloc();
        m_inode->i_di.i_no = ino;
        efs_i_addsb(m_inode);
        // 这里由于一定是第一次的，且是互斥查找，并不会造成睡眠，
        // 只是为了挡住其他查找到但是 inode 没有初始化完的并行线程
        sleep_on(&m_inode->i_slock);
        creating = 1;
        // printk("2\n");
    }
    spin_unlock(&m_esb.s_lock);

    if (creating == 1) {
        // 上面拿到睡眠锁的线程来初始化
        efs_i_fill(m_inode, ino);

        SET_FLAG(&m_inode->i_flags, I_VALID);
        wake_up(&m_inode->i_slock);
        // printk("4\n");
    }
    // 如果当前这个无效，但是在哈希表中找到了，说明正在被创建
    else if (!TEST_FLAG(&m_inode->i_flags, I_VALID)) {
        // printk("5\n");
        sleep_on(&m_inode->i_slock);  // 会在这里等待某个线程把这个 inode 初始化完成
        wake_up(&m_inode->i_slock);
        // printk("6\n");
    }

    atomic_inc(&m_inode->i_refcnt);
    return m_inode;
}

// 读 inode 指向的文件的 offset len 信息
int efs_i_read(struct easy_m_inode *inode, uint32_t offset, uint32_t len, void *vaddr)
{
    int bno;
    uint32_t tot;  // 总共读的字节数 <= len
    uint32_t m;    // 每一次读的字节数

    if (offset >= inode->i_di.i_size || offset + len < offset)
        return 0;
    // 越界
    if (offset + len > inode->i_di.i_size)
        len = inode->i_di.i_size - offset;

    // 对应的物理块实际上大可能不连续，实际需要按块来读
    sleep_on(&inode->i_slock);
    for (tot = 0; tot < len; tot += m, offset += m, vaddr = (char *)vaddr + m) {
        bno = efs_i_bmap(inode, offset / BLOCK_SIZE, 0);
        if (bno == 0)
            break;
        m = min(len - tot, BLOCK_SIZE - offset % BLOCK_SIZE);
        blk_read(efs_bd, bno, (offset % BLOCK_SIZE), m, vaddr);
    }
    wake_up(&inode->i_slock);
    return tot;
}

int efs_i_write(struct easy_m_inode *inode, uint32_t offset, uint32_t len, void *vaddr)
{
    int bno;
    uint32_t tot, m;
    if (offset > inode->i_di.i_size || offset + len < offset)
        return 0;
    if (offset + len >= MAXFILE * BLOCK_SIZE)
        return -1;
    sleep_on(&inode->i_slock);
    for (tot = 0; tot < len; tot += m, offset += m, vaddr = (char *)vaddr + m) {
        bno = efs_i_bmap(inode, offset / BLOCK_SIZE, 1);
        if (bno == 0)
            break;
        m = min(len - tot, BLOCK_SIZE - offset % BLOCK_SIZE);
        blk_write(efs_bd, bno, (offset % BLOCK_SIZE), m, vaddr);
    }

    spin_lock(&m_esb.s_lock);
    spin_lock(&inode->i_lock);
    if (offset > inode->i_di.i_size)
        inode->i_di.i_size = offset;
    efs_i_sdirty(inode);
    spin_unlock(&inode->i_lock);
    spin_unlock(&m_esb.s_lock);

    wake_up(&inode->i_slock);
    return tot;
}

// 释放数据块，需要对超级块加自旋锁、i_slock
static void efs_i_data_free(struct easy_m_inode *inode)
{
    int i;
    for (i = 0; i < NDIRECT + 1; i++) {
        if (!inode->i_di.i_addrs[i])
            break;
        efs_bmap_free(inode->i_di.i_addrs[i]);
    }
    if (inode->i_indir) {
        for (i = 0; i < NINDIRECT; i++) {
            if (!inode->i_indir[i])
                break;
            efs_bmap_free(inode->i_indir[i]);
        }
    }
}

// 截断inode（丢弃内容）。
// 删除了这个Inode指向的文件，但是并没有删除inode本身
void efs_i_trunc(struct easy_m_inode *i)
{
    // 释放数据块

    sleep_on(&i->i_slock);
    spin_lock(&m_esb.s_lock);
    efs_i_data_free(i);  // 释放数据块
    spin_unlock(&m_esb.s_lock);
    wake_up(&i->i_slock);

    spin_lock(&m_esb.s_lock);
    spin_lock(&i->i_lock);
    i->i_di.i_size = 0;
    efs_i_sdirty(i);
    spin_unlock(&i->i_lock);
    spin_unlock(&m_esb.s_lock);
}

// 申请一个新的 inode,理论上后面初始化不会发生竞争
struct easy_m_inode *efs_i_new()
{
    int ino;
    spin_lock(&m_esb.s_lock);
    ino = efs_imap_alloc();
    // printk("efs_i_new ino: %d\n", ino);
    spin_unlock(&m_esb.s_lock);

    struct easy_m_inode *inode = efs_i_alloc();
    assert(inode != NULL, "efs_i_new\n");

    struct easy_d_inode *d_inode = &inode->i_di;
    d_inode->i_no = ino;
    d_inode->i_type = F_NONE;
    d_inode->i_devno = efs_bd->bd_dev;
    d_inode->i_size = 0;
    memset(d_inode->i_addrs, '\0', sizeof(d_inode->i_addrs));
    atomic_set(&d_inode->i_nlink, 1);

    atomic_set(&inode->i_refcnt, 0);

    spin_lock(&m_esb.s_lock);
    // 对于一个新的 inode，在加入哈希表之前，其他线程获取不到这个 inode
    // 即标记为脏可以不用对 inode 加锁
    efs_i_sdirty(inode);
    efs_i_addsb(inode);

    spin_unlock(&m_esb.s_lock);
    return inode;
}

// 减少 inode 的链接数，当链接数为 0 时释放空间
int efs_i_unlink(struct easy_m_inode *inode)
{
    if (!inode)
        return -1;
    if (inode->i_di.i_no <= 1) {
        printk("efs_i_unlink inode->i_di.i_no <= 1\n");
        return -1;
    }

    if (atomic_dec_and_test(&inode->i_di.i_nlink)) {
        // 释放
        // 如果有进程正在使用，即引用计数不为0，此时不能释放
        if (atomic_read(&inode->i_refcnt) > 0) {
            printk("efs_i_unlink inode: %d, ref: %d > 0\n", inode->i_di.i_no, atomic_read(&inode->i_refcnt));
            atomic_inc(&inode->i_di.i_nlink);
            return -1;
        }
        // 删除

        // 1. 移除相关内存结构, 已经在 put 后 free 实现
        // list_del(&inode->i_dirty);
        // list_del(&inode->i_list);
        // hash_del_node(&inode->i_sb->s_ihash, &inode->i_hnode);

        // 2. 释放数据块
        efs_i_trunc(inode);

        // 3. imap
        spin_lock(&m_esb.s_lock);
        efs_imap_free(inode->i_di.i_no);
        spin_unlock(&m_esb.s_lock);
    }
    return 0;
}

inline int efs_i_size(struct easy_m_inode *inode)
{
    return inode->i_di.i_size;
}

void efs_i_root_init()
{
    int offset = offset_ino(1);
    blk_read(efs_bd, m_esb.s_ds.inode_area_start, offset, sizeof(struct easy_d_inode), &root_m_inode.i_di);
    assert(root_m_inode.i_di.i_no == 1, "root_m_inode.i_di.i_no !=1");
    root_m_inode.i_flags = 0;
    // spin_init(&m_inode->i_lock, "i_lock");
    sleep_init(&root_m_inode.i_slock, "i_slock");
    atomic_set(&root_m_inode.i_refcnt, 1);
    root_m_inode.i_sb = &m_esb;
    INIT_HASH_NODE(&root_m_inode.i_hnode);
    INIT_LIST_HEAD(&root_m_inode.i_list);
    INIT_LIST_HEAD(&root_m_inode.i_dirty);
    SET_FLAG(&root_m_inode.i_flags, I_VALID);

    efs_i_addsb(&root_m_inode);
}

inline void efs_i_type(struct easy_m_inode *inode, enum easy_file_type ftype)
{
    spin_lock(&m_esb.s_lock);
    spin_lock(&inode->i_lock);

    inode->i_di.i_type = ftype;
    efs_i_sdirty(inode);

    spin_unlock(&inode->i_lock);
    spin_unlock(&m_esb.s_lock);
}

__attribute__((unused)) void efs_i_info(const struct easy_m_inode *inode)
{
    const struct easy_d_inode *d_inode = &inode->i_di;
    printk("easy-fs easy_inode infomation:\n");
    printk("  no: %d\n", d_inode->i_no);
    printk("  type: %d\n", d_inode->i_type);
    printk("  devno: %d\n", d_inode->i_devno);
    printk("  size: %d\n", d_inode->i_size);

    int i;
    for (i = 0; i < NDIRECT; i++) {
        if (!inode->i_di.i_addrs[i])
            break;
        printk("%d -> bno: %d\n", i, inode->i_di.i_addrs[i]);
    }
    if (inode->i_indir) {
        for (i = 0; i < NINDIRECT; i++) {
            if (!inode->i_indir[i])
                break;
            printk("%d -> bno: %d\n", i + NDIRECT, inode->i_indir[i]);
        }
    }

    printk("  nlink: %d\n", d_inode->i_nlink);

    printk("  --mem--\n");
    printk("  flags: %d\n", inode->i_flags);
    printk("  ref: %d\n", atomic_read(&inode->i_refcnt));
}
