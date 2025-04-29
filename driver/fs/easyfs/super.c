#include "easyfs.h"
#include "mm/mm.h"
#include "std/string.h"
#include "lib/bitmap.h"
#include "dev/blk/blk_dev.h"
#include "core/proc.h"

struct easy_m_super_block m_esb;

static struct bitmap imap;
static struct bitmap bmap;

extern int efs_i_update(struct easy_m_inode *m_inode);
extern void efs_i_cdirty(struct easy_m_inode *inode);
extern void efs_d_update(struct easy_dentry *pd);
extern void efs_d_cdirty(struct easy_dentry *d);
static void efs_fill_imap() {
    // 这个位图同下 bmap，目前感觉一个块完全够了，理论最大支持 4096 * 8 = 32,768 个文件
    uint64 *imap_addr = __alloc_page(0);
    assert(imap_addr != NULL, "efs_fill_imap\n");
    blk_read_count(efs_bd, m_esb.s_ds.inode_map_start, m_esb.s_ds.inode_area_start - m_esb.s_ds.inode_map_start, imap_addr);
    bitmap_init_zone(&imap, imap_addr, m_esb.s_ds.inode_count);
    bitmap_reset_start(&imap, 1);
    bitmap_reset_unused(&imap, m_esb.s_ds.inode_free);
}

static void efs_fill_bmap() {
    // 暂时很不幸，我们直接写死，目前最大支持 2 GB （2^4 * 4096 * 8 * BLOCK_SIZE / 1024/1024/1024）
    uint64 *bmap_addr = __alloc_pages(0, 4);
    assert(bmap_addr != NULL, "efs_fill_bmap\n");
    blk_read_count(efs_bd, m_esb.s_ds.block_map_start, m_esb.s_ds.block_area_start - m_esb.s_ds.block_map_start, bmap_addr);
    bitmap_init_zone(&bmap, bmap_addr, m_esb.s_ds.block_count);
    bitmap_reset_start(&bmap, 1);
    bitmap_reset_unused(&bmap, m_esb.s_ds.block_free);
}

// // 挂载 root
// inline void efs_mount_root(struct easy_m_inode *root_inode, struct easy_dentry *root_dentry)
// {
//     m_esb.s_rooti = root_inode;
//     m_esb.s_rootd = root_dentry;
//     root_inode->i_sb = &m_esb;

//     list_add_head(&m_esb.s_ilist, &root_inode->i_list);
//     list_add_head(&m_esb.s_dlist, &root_dentry->d_list);

//     // inode hash
//     hash_add_head(&m_esb.s_ihash, root_inode->i_no, &root_inode->i_hnode);

//     // dentry hash
//     hash_add_head(&m_esb.s_dhash, efs_hashd(root_dentry), &root_dentry->d_hnode);
// }

// 从磁盘读超级块用于初始化
// 只会调用一次，由 efs_sb_init 调用，且不需要加锁
static int efs_sb_read() {
    return blk_read(efs_bd, SUPER_BLOCK_LOCATION, 0, sizeof(m_esb.s_ds), &m_esb.s_ds);
}

// 填充超级块
static void efs_sb_fill() {
    if (m_esb.s_ds.magic != EASYFS_MAGIC)
        panic("File system magic number error!\n");

    m_esb.s_bd    = efs_bd;
    m_esb.s_flags = 0;

    hash_init(&m_esb.s_ihash, 41, "easy_i_hash");
    hash_init(&m_esb.s_dhash, 41, "easy_d_hash");
    INIT_LIST_HEAD(&m_esb.s_ilist);
    INIT_LIST_HEAD(&m_esb.s_dlist);
    INIT_LIST_HEAD(&m_esb.s_idirty_list);
    INIT_LIST_HEAD(&m_esb.s_ddirty_list);
    m_esb.s_rooti = NULL;
    m_esb.s_rootd = NULL;

    spin_init(&m_esb.s_lock, "sb-lock");
    sleep_init(&m_esb.s_sleep_lock, "sb-sleep");

    efs_fill_imap();
    efs_fill_bmap();
}

// 写回超级块(更新) 需要持有 m_esb s_sleep_lock

static __attribute__((noreturn)) void efs_sync() {
#ifdef DEBUG_EFS_SYNC
    int i_dirty = 0;
    int d_dirty = 0;
    printk("EFS_SYNC start...\n");
#endif
    while (1) {
        thread_timer_sleep(myproc(), 2000);
        spin_lock(&m_esb.s_lock);
        if (TEST_FLAG(&m_esb.s_flags, S_DIRTY)) {
            spin_unlock(&m_esb.s_lock);
            // sb & imap & iarea & bmap & barea
            blk_write(efs_bd, SUPER_BLOCK_LOCATION, 0, sizeof(m_esb.s_ds), &m_esb.s_ds);
            blk_write_count(efs_bd, m_esb.s_ds.inode_map_start, m_esb.s_ds.inode_area_start - m_esb.s_ds.inode_map_start, imap.map);
            blk_write_count(efs_bd, m_esb.s_ds.block_map_start, m_esb.s_ds.block_area_start - m_esb.s_ds.block_map_start, bmap.map);

            spin_lock(&m_esb.s_lock);
            CLEAR_FLAG(&m_esb.s_flags, S_DIRTY);
            // wake_up(&m_esb.s_sleep_lock);

#ifdef DEBUG_EFS_SYNC
            printk("efs sync sb\n");
#endif
        }

        // 刷新脏的 inode 到磁盘
        while (!list_empty(&m_esb.s_idirty_list)) {
            struct easy_m_inode *i =
                list_entry(list_pop(&m_esb.s_idirty_list), struct easy_m_inode, i_dirty);
#ifdef DEBUG_EFS_SYNC
            i_dirty++;
#endif
            spin_unlock(&m_esb.s_lock);

            sleep_on(&i->i_slock);
            efs_i_update(i);
            wake_up(&i->i_slock);

            spin_lock(&m_esb.s_lock);

            spin_lock(&i->i_lock);
            efs_i_cdirty(i);
            spin_unlock(&i->i_lock);
        }
#ifdef DEBUG_EFS_SYNC
        if (i_dirty != 0)
            printk("efs sync dirty inode \tcount: %d\n", i_dirty);
        i_dirty = 0;
#endif

        // 刷新目录文件
        while (!list_empty(&m_esb.s_ddirty_list)) {
            struct easy_dentry *d = list_entry(list_pop(&m_esb.s_ddirty_list), struct easy_dentry, d_dirty);
#ifdef DEBUG_EFS_SYNC
            d_dirty++;
#endif
            spin_unlock(&m_esb.s_lock);

            sleep_on(&d->d_slock);
            efs_d_update(d);
            wake_up(&d->d_slock);

            spin_lock(&m_esb.s_lock);

            spin_lock(&d->d_lock);
            efs_d_cdirty(d);
            spin_unlock(&d->d_lock);
        }

#ifdef DEBUG_EFS_SYNC
        if (d_dirty != 0)
            printk("efs sync dirty dentry \tcount: %d\n", d_dirty);
        d_dirty = 0;
#endif

        spin_unlock(&m_esb.s_lock);
    }
}

// 超级块初始化
void efs_sb_init() {
    efs_sb_read();
    efs_sb_fill();
    kthread_create(myproc(), efs_sync, NULL, "efs_sync", NO_CPU_AFF);
}

// 上面的函数都是初始化时使用，内核中已经确保不需要加锁。

// 在 inode 位图中分配 需要持有 m_esb lock
int efs_imap_alloc() {
    int no = bitmap_alloc(&imap);
    if (no == -1)
        panic("inode_no_alloc\n");
    m_esb.s_ds.inode_free--;
    SET_FLAG(&m_esb.s_flags, S_DIRTY);
    return no;
}

// 在 inode 位图中回收 需要持有 m_esb lock
void efs_imap_free(int ino) {
    if (bitmap_is_free(&imap, ino))
        panic("efs_imap_free\n");
    m_esb.s_ds.inode_free++;
    bitmap_free(&imap, ino);
    SET_FLAG(&m_esb.s_flags, S_DIRTY);
}

// 在块位图中分配 需要持有 m_esb lock
int efs_bmap_alloc() {
    int no = bitmap_alloc(&bmap);
    if (no == -1)
        panic("data_block_alloc\n");
    m_esb.s_ds.block_free--;
    SET_FLAG(&m_esb.s_flags, S_DIRTY);
    return no;
}

// 在块位图中回收一个 需要持有 m_esb lock
void efs_bmap_free(int bno) {
    if (bitmap_is_free(&bmap, bno))
        panic("efs_bmap_free\n");
    m_esb.s_ds.block_free++;
    bitmap_free(&bmap, bno);
    SET_FLAG(&m_esb.s_flags, S_DIRTY);
}

// 显示超级块信息
__attribute__((unused)) void efs_sb_info() {
    printk("easy_super_block information:\n");
    struct easy_d_super_block *ds = &m_esb.s_ds;

    // 打印 inode 相关信息
    printk("  inode_count: %d\n", ds->inode_count);
    printk("  inode_free: %d\n", ds->inode_free);
    printk("  inode_size: %d\n", ds->inode_size);
    printk("  inode_map_start: %d\n", ds->inode_map_start);
    printk("  inode_area_start: %d\n", ds->inode_area_start);

    // 打印 block 相关信息
    printk("  block_count: %d\n", ds->block_count);
    printk("  block_free: %d\n", ds->block_free);
    printk("  block_size: %d\n", ds->block_size);
    printk("  block_map_start: %d\n", ds->block_map_start);
    printk("  block_area_start: %d\n", ds->block_area_start);

    printk("  name: %s\n", ds->name);
    printk("  magic: %p\n", ds->magic);
    printk("  imap:\n");
    bitmap_info(&imap, 0);
    printk("  bmap:\n");
    bitmap_info(&bmap, 0);
}
