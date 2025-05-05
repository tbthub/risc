#include "easyfs.h"
#include "std/string.h"
#include "dev/blk/blk_dev.h"
#include "mm/kmalloc.h"

extern void efs_i_dup(struct easy_m_inode *inode);

static const char t[]          = "\0-db"; // t[1] = '-', t[2] = 'd', t[3] = 'b'
struct easy_dentry root_dentry = {
    .d_dd.d_ino  = 1,
    .d_dd.d_name = "/",
    .d_dd.d_type = F_DIR,
};

extern void efs_i_trunc(struct easy_m_inode *inode);

// *
// ! 锁 锁  不支持挂载
// ?
// TODO

/*
 * 这里简单实现目录功能，但是效率比较低
 *
 * 目录文件在磁盘的存放内容我们简单实现为 easy_dirent[]
 * 1. D：将其从 dentry 链移除
 * 2. C：插入到 dentry 链
 * 3. R：遍历 dentry 链
 * 4. U: 直接该即可
 * 目录文件修改后会写回磁盘，后面就由 flush 控制了
 *
 */

// 将文件类型转换为便于理解的字符 - d b
static const char efs_d_type_tostr(enum easy_file_type type) {
    return t[type]; // 直接返回 t 数组的地址
}

static inline int efs_d_namecmp(const char *s, const char *t) {
    return strncmp(s, t, DIR_MAXLEN);
}

// 修改 name、type、增、删时标记为脏
// 需要超级块的锁
static void efs_d_sdirty(struct easy_dentry *d) {
    SET_FLAG(&d->d_flags, D_DIRTY);
    if (list_empty(&d->d_dirty))
        list_add_head(&d->d_dirty, &m_esb.s_ddirty_list);
}

// 需要超级块的锁
void efs_d_cdirty(struct easy_dentry *d) {
    CLEAR_FLAG(&d->d_flags, D_DIRTY);
    list_del_init(&d->d_dirty);
}

// 把 dentry 加到超级块链表中,需要加锁
static inline void efs_d_addsb(struct easy_dentry *d) {
    hash_add_head(&m_esb.s_dhash, strhash(d->d_dd.d_name), &d->d_hnode);
    list_add_head(&d->d_list, &m_esb.s_dlist);
}

static struct easy_dentry *efs_d_alloc() {
    struct easy_dentry *d = kmem_cache_alloc(&efs_dentry_kmem_cache);
    assert(d != NULL, "efs_d_alloc");

    d->d_inode  = NULL;
    d->d_parent = NULL;
    INIT_LIST_HEAD(&d->d_child);
    INIT_LIST_HEAD(&d->d_sibling);

    INIT_HASH_NODE(&d->d_hnode);
    INIT_LIST_HEAD(&d->d_list);
    INIT_LIST_HEAD(&d->d_dirty);

    spin_init(&d->d_lock, "dentry");
    sleep_init(&d->d_slock, "dentry");

    d->d_flags = 0;
    atomic_set(&d->refcnt, 0);
    return d;
}

// 关联父子目录项
static inline void efs_d_conn(struct easy_dentry *d, struct easy_dentry *pd) {
    d->d_parent = pd;
    list_add_tail(&d->d_sibling, &pd->d_child);
}

// * 从磁盘中将目录的信息读出，填充内存中的目录项
// 需要睡眠锁
// @param pd: 父目录项
static void efs_d_fill(struct easy_dentry *pd) {
    // 先获取到对应的 inode
    struct easy_m_inode *pi = efs_i_get(pd->d_dd.d_ino);
    pd->d_inode             = pi;
    int d_cnt               = pi->i_di.i_size / sizeof(struct easy_dirent);
    for (int i = 0; i < d_cnt; i++) {
        struct easy_dentry *d = efs_d_alloc();
        efs_i_read(pi, i * sizeof(struct easy_dirent), sizeof(struct easy_dirent), &d->d_dd);
        efs_d_conn(d, pd);

        spin_lock(&m_esb.s_lock);
        efs_d_addsb(d);
        spin_unlock(&m_esb.s_lock);
    }
    efs_i_put(pi);
}

// TODO 结合 inode 描述的性能问题
// 将目录文件内容写回磁盘
void efs_d_update(struct easy_dentry *pd) {
    struct easy_dentry *d;
    int offset = 0;

    if (!pd->d_inode) {
        pd->d_inode = efs_i_get(pd->d_dd.d_ino);
        efs_i_put(pd->d_inode);
    }

    list_for_each_entry(d, &pd->d_child, d_sibling) {
        efs_i_write(pd->d_inode, offset, sizeof(struct easy_dirent), &d->d_dd);
        offset += sizeof(struct easy_dirent);
    }
}

// * 查重 需要加锁
// 返回值 重复对象的目录项指针。 NULL：通过  else：未通过（含有同名）
static struct easy_dentry *efs_d_name_check(struct easy_dentry *pd, const char *name) {
    struct easy_dentry *d = NULL;
    if (!TEST_FLAG(&pd->d_flags, D_CHILD)) // 如果没有填充子目录
    {
        efs_d_fill(pd);
        SET_FLAG(&pd->d_flags, D_CHILD);
    }

    list_for_each_entry(d, &pd->d_child, d_sibling) {
        if (efs_d_namecmp(d->d_dd.d_name, name) == 0)
            return d;
    }
    return NULL;
}

// 返回子目录里面 name 的目录项 需要加锁
static inline struct easy_dentry *efs_d_namex(struct easy_dentry *pd, const char *name) {
    return efs_d_name_check(pd, name);
}

// path 没有映射，会从磁盘读取
struct easy_dentry *efs_d_named(const char *path) {
    struct easy_dentry *d;
    struct easy_dentry *dtmp;
    char *p, *tmp, *name;

    // 首先查找 dentry 缓存
    spin_lock(&m_esb.s_lock);
    hash_for_each_entry(d, &m_esb.s_dhash, strhash(path), d_hnode) {
        // 如果找到了这个 dentry
        if (!efs_d_namecmp(d->d_dd.d_name, path)) {
            spin_unlock(&m_esb.s_lock);
            return d;
        }
    }
    spin_unlock(&m_esb.s_lock);

    tmp = p         = kmalloc(MAX_PATH_LEN + 1, 0);
    p[MAX_PATH_LEN] = '\0';
    strncpy(p, path, MAX_PATH_LEN);

    // 如果是绝对路径
    // TODO 暂时只支持绝对路径，以后改
    if (p[0] == '/') {
        d = &root_dentry;
        p++;
    }

    else // 如果是相对路径
    {
        // TODO 相对路径：从进程的 pwd 开始
    }
    name = p;

    while (*p) {
        if (*p == '/') {
            *p = '\0'; // 截断
            sleep_on(&d->d_slock);
            dtmp = efs_d_namex(d, name);
            wake_up(&d->d_slock);
            if (!dtmp)
                return NULL;
            d    = dtmp;
            name = p + 1; // 移动到下一个目录名
        }
        p++;
    }

    // 解析最后一级目录
    if (*name) {
        sleep_on(&d->d_slock);
        dtmp = efs_d_namex(d, name);
        wake_up(&d->d_slock);
        d = dtmp;
    }

    kfree(tmp);
    return d;
}

// 根据 path 返回 inode,可能会睡眠。
// 已增加引用计数
struct easy_m_inode *efs_d_namei(const char *path) {
    struct easy_m_inode *i;
    struct easy_dentry *d;

    d = efs_d_named(path);
    if (!d) {
        printk("efs_d_namei: file not found: %s\n", path);
        return NULL;
    }

    if (d->d_inode)
        efs_i_dup(d->d_inode);
    else
        d->d_inode = efs_i_get(d->d_dd.d_ino);

    i = d->d_inode;
    return i;
}

// * (这个函数可能会陷入睡眠)
void efs_d_lookup(struct easy_dentry *pd) {
    struct easy_dentry *d = NULL;
    sleep_on(&pd->d_slock);
    if (!TEST_FLAG(&pd->d_flags, D_CHILD)) // 如果没有填充子目录
    {
        efs_d_fill(pd);
        SET_FLAG(&pd->d_flags, D_CHILD);
    }

    list_for_each_entry(d, &pd->d_child, d_sibling)
        printk("%c\t%s\n", efs_d_type_tostr(d->d_dd.d_type), d->d_dd.d_name);

    wake_up(&pd->d_slock);
}

struct easy_dentry *efs_d_creat(struct easy_dentry *pd, const char *name, enum easy_file_type type) {
    if (!pd) {
        printk("efs_d_creat: pd is NULL\n");
        return NULL;
    }
    if (pd->d_dd.d_type != F_DIR) {
        printk("efs_d_creat: %s is not a dir!\n", pd->d_dd.d_name);
        return NULL;
    }

    sleep_on(&pd->d_slock);
    if (efs_d_name_check(pd, name)) {
        printk("efs_d_creat: cannot create directory '%s': File exists\n", name);
        wake_up(&pd->d_slock);
        return NULL;
    }

    struct easy_m_inode *new_i = efs_i_new();
    struct easy_dentry *new_d  = efs_d_alloc();

    new_d->d_dd.d_ino  = new_i->i_di.i_no;
    new_d->d_dd.d_type = type;
    strncpy(new_d->d_dd.d_name, name, DIR_MAXLEN);
    efs_i_type(new_i, type);

    efs_d_conn(new_d, pd);

    spin_lock(&m_esb.s_lock);

    spin_lock(&pd->d_lock);
    pd->d_inode->i_di.i_size += sizeof(struct easy_dirent);
    efs_d_sdirty(pd);
    spin_unlock(&pd->d_lock);

    spin_unlock(&m_esb.s_lock);

    wake_up(&pd->d_slock);

    return new_d;
}

void efs_d_unlink(struct easy_dentry *d) {
    if (!d) {
        printk("efs_d_unlink: The file to be deleted does not exist\n");
        return;
    }

    if (d->d_dd.d_type == F_DIR) {
        if (!TEST_FLAG(&d->d_flags, D_CHILD)) // 如果没有填充子目录
        {
            efs_d_fill(d);
            SET_FLAG(&d->d_flags, D_CHILD);
        }
        if (!list_empty(&d->d_child)) {
            printk("efs_d_unlink: failed to remove '%s': Directory not empty\n", d->d_dd.d_name);
            return;
        }
    }

    if (d->d_dd.d_ino == 1) {
        printk("efs_d_unlink: The root directory cannot be deleted\n");
        return;
    }

    if (d->d_inode)
        efs_i_dup(d->d_inode);
    else
        d->d_inode = efs_i_get(d->d_dd.d_ino);
    efs_i_put(d->d_inode);

    if (efs_i_unlink(d->d_inode) == 0) {
        list_del_init(&d->d_sibling);

        spin_lock(&m_esb.s_lock);
        spin_lock(&d->d_lock);

        d->d_parent->d_inode->i_di.i_size -= sizeof(struct easy_dirent);
        efs_d_sdirty(d->d_parent);

        spin_unlock(&d->d_lock);
        spin_unlock(&m_esb.s_lock);
    }
}

void efs_d_rename(struct easy_dentry *d, const char *rename) {
    if (efs_d_name_check(d->d_parent, rename)) {
        printk("efs_d_rename: cannot create directory '%s': File exists\n", rename);
        return;
    }

    if (d->d_inode)
        efs_i_dup(d->d_inode);
    else
        d->d_inode = efs_i_get(d->d_dd.d_ino);

    spin_lock(&d->d_lock);
    strncpy(d->d_dd.d_name, rename, DIR_MAXLEN);
    spin_unlock(&d->d_lock);

    efs_i_put(d->d_inode);
}

// 目录对应的实际文件内容读写操作,需要对其加睡眠锁。。因为。。我们没有实现读写锁
// dup，namei 会增加引用计数，put 释放。
// 在中间对 inode 内容读写操作，可以避免读写时有其他线程删除文件，从而导致未知错误
// 我们允许目录项目被删除，这个 inode 暂存。之后这个 inode 也会被回收
int efs_d_read(struct easy_dentry *d, uint32 offset, uint32 len, void *vaddr) {
    if (!d) {
        printk("efs_d_read: file not found\n");
        return -1;
    }
    sleep_on(&d->d_slock);
    if (!d->d_inode) {
        d->d_inode = efs_i_get(d->d_dd.d_ino);
        efs_i_put(d->d_inode);
    }
    efs_i_dup(d->d_inode);

    int res = efs_i_read(d->d_inode, offset, len, vaddr);
    efs_i_put(d->d_inode);
    wake_up(&d->d_slock);
    return res;
}

int efs_d_read_name(const char *path, uint32 offset, uint32 len, void *vaddr) {
    struct easy_dentry *d;
    d = efs_d_named(path);
    if (!d) {
        printk("efs_d_read_name: file '%s' not found\n", path);
        return -1;
    }
    return efs_d_read(d, offset, len, vaddr);
}

int efs_d_write(struct easy_dentry *d, uint32 offset, uint32 len, void *vaddr) {
    if (!d) {
        printk("efs_d_write: file not found\n");
        return -1;
    }
    if (d->d_dd.d_type == F_DIR) {
        printk("efs_d_write: %s is a dir\n", d->d_dd.d_name);
        return -1;
    }
    sleep_on(&d->d_slock);
    if (!d->d_inode) {
        d->d_inode = efs_i_get(d->d_dd.d_ino);
        efs_i_put(d->d_inode);
    }
    efs_i_dup(d->d_inode);

    int res = efs_i_write(d->d_inode, offset, len, vaddr);
    efs_i_put(d->d_inode);

    wake_up(&d->d_slock);
    return res;
}

int efs_d_write_name(const char *path, uint32 offset, uint32 len, void *vaddr) {
    struct easy_dentry *d;
    d = efs_d_named(path);
    if (!d) {
        printk("efs_d_write_name: file '%s' not found\n", path);
        return -1;
    }
    return efs_d_write(d, offset, len, vaddr);
}

int efs_d_fsize(struct easy_dentry *d) {
    if (d->d_inode)
        efs_i_dup(d->d_inode);
    else
        d->d_inode = efs_i_get(d->d_dd.d_ino);
    int fsize = efs_i_size(d->d_inode);
    efs_i_put(d->d_inode);
    return fsize;
}

// // 删除目录项
// int efs_d_rm(struct easy_dentry *d)
// {
//     if (d->d_dd.d_type == F_DIR && !list_empty(&d->d_child))
//     {
//         printk("efs_d_rm: failed to remove '%d': Directory not empty", d->d_dd.d_name);
//         return 0;
//     }
//     // 删除 inode
// }

// // 移除相关内存结构，不过并没有释放 inode
// static void efs_d_free(struct easy_dentry *d)
// {
//     if (!d)
//         return;

//     spin_lock(&m_esb.s_lock);

//     list_del(&d->d_list);
//     hash_del_node(&m_esb.s_dhash, &d->d_hnode);

//     spin_unlock(&m_esb.s_lock);

//     list_del(&d->d_child);
//     list_del(&d->d_sibling);

//     kmem_cache_free(&efs_dentry_kmem_cache, d);
// }

// int efs_d_free(struct easy_dentry *d)
// {
//     if (!d)
//         return;
// }

// int efs_d_unlink(struct easy_dentry *pd, struct easy_dentry *d)
// {
//     if (!pd || !d)
//         return -1; // 目标不存在

//     if (d->d_dd.d_type == F_DIR && !list_empty(&d->d_child))
//         return -1; // 不能删除非空目录

//     // 从父目录的子目录链表中移除
//     list_del(&d->d_sibling);

//     // 释放 inode 和 dentry 资源
//     efs_i_del(d->d_inode);

//     return 0; // 删除成功
// }

// * 必须在 efs_i_root_init 之后
// 这个不存在竞争
void efs_d_root_init() {
    assert(root_m_inode.i_sb != NULL, "efs_d_root_init\n");

    root_dentry.d_inode  = NULL;
    root_dentry.d_parent = NULL;
    INIT_LIST_HEAD(&root_dentry.d_child);
    INIT_LIST_HEAD(&root_dentry.d_sibling);

    INIT_HASH_NODE(&root_dentry.d_hnode);
    INIT_LIST_HEAD(&root_dentry.d_list);
    INIT_LIST_HEAD(&root_dentry.d_dirty);

    spin_init(&root_dentry.d_lock, "dentry");
    sleep_init(&root_dentry.d_slock, "dentry");

    root_dentry.d_flags = 0;
    efs_d_fill(&root_dentry);
    SET_FLAG(&root_dentry.d_flags, D_CHILD);

    efs_d_addsb(&root_dentry);
}

static void __efs_d_infos(struct easy_dentry *d, int indent) {

    // 打印当前目录项
    for (int i = 0; i < indent; i++)
        printk(" ");
    printk("%s\n", d->d_dd.d_name); // 目录后面加 "/" 以区分
    // 确保目录已填充子项
    if (d->d_dd.d_type == F_DIR) {
        if (!TEST_FLAG(&d->d_flags, D_CHILD)) {
            efs_d_fill(d);
            SET_FLAG(&d->d_flags, D_CHILD);
        }
    }

    // 遍历子目录项
    struct easy_dentry *d1;
    list_for_each_entry(d1, &d->d_child, d_sibling) {
        if (d1->d_dd.d_type == F_DIR) {
            // 递归打印子目录
            __efs_d_infos(d1, indent + 3);
        } else {
            // 打印普通文件
            for (int i = 0; i < indent + 3; i++)
                printk(" ");
            printk("%s\n", d1->d_dd.d_name);
        }
    }
}

// * 用于调试
void efs_d_infos(struct easy_dentry *d) {
    if (!d) {
        printk("efs_d_infos The given dentry is NULL\n");
        return;
    }
    __efs_d_infos(d, 0);
}

void efs_d_info(struct easy_dentry *d) {
    if (!d)
        return;
    printk("%c\t%s\n", efs_d_type_tostr(d->d_dd.d_type), d->d_dd.d_name);
}
