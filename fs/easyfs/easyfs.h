#ifndef __efs_H__
#define __efs_H__
#include "std/stddef.h"
#include "lib/hash.h"
#include "lib/list.h"
#include "lib/sleeplock.h"
#include "dev/blk/blk_dev.h"

#define EASYFS_MAGIC 0x12345678
#define BLOCK_SIZE (4 * 1024)
#define INODE_SIZE 128
#define NDIRECT 12
#define NINDIRECT (BLOCK_SIZE / sizeof(int))
#define MAXFILE (NDIRECT + NINDIRECT)

#define SUPER_BLOCK_LOCATION 1
#define AVG_FILE_SIZE (500 * 1024) // 假设平均文件大小为 500KB

enum easy_file_type
{
    F_NONE,
    F_REG,
    F_DIR,
    F_DEV,
};

// 磁盘上的超级块
struct easy_d_super_block
{
    int inode_count;      // 总 inode 数
    int inode_free;       // 可用 inode 数
    int inode_size;       // inode 大小
    int inode_map_start;  // inode 位图
    int inode_area_start; // inode 数据

    int block_count;      // 总块数
    int block_free;       // 可用块数
    int block_size;       // 块大小
    int block_map_start;  // 块位图
    int block_area_start; // 块数据

    char name[16];

    uint32 magic;
};

#define S_DIRTY (1 << 0) // 脏标志，用于定期同步
// 内存上的超级块
struct easy_m_super_block
{
    struct easy_d_super_block s_ds;

    // 内存中的超级块含有的额外信息
    struct block_device *s_bd;
    flags_t s_flags;

    struct hash_table s_ihash;
    struct hash_table s_dhash;

    struct list_head s_ilist;
    struct list_head s_dlist;

    struct list_head s_idirty_list;
    struct list_head s_ddirty_list;

    struct easy_m_inode *s_rooti;
    struct easy_dentry *s_rootd;

    spinlock_t s_lock;
    sleeplock_t s_sleep_lock;
};

// -----------------------------------------------

// 磁盘上的 inode
struct easy_d_inode
{
    uint32 i_no;
    enum easy_file_type i_type;
    uint16 i_devno;              // 如果是设备文件,标识设备号
    uint32 i_size;               // 文件大小
    uint32 i_addrs[NDIRECT + 1]; // 12个直接 + 1 个间接

    atomic_t i_nlink; // 链接数量，用于支持硬链接
};

#define I_DIRTY (1 << 0)
#define I_VALID (1 << 1)

// 内存上的 inode
struct easy_m_inode
{
    struct easy_d_inode i_di;

    sleeplock_t i_slock; // 睡眠锁控制与磁盘 IO 互斥

    atomic_t i_refcnt; // 引用计数

    spinlock_t i_lock; // 自旋锁保护下面的属性
    flags_t i_flags;

    struct easy_m_super_block *i_sb;

    hash_node_t i_hnode; // 根据 inode_no 哈希
    struct list_head i_list;
    struct list_head i_dirty;

    uint32 *i_indir;
};

// -----------------------------------------------

#define DIR_MAXLEN 16

#define D_CHILD (1 << 0) // 是否已经把子目录读入
#define D_DIRTY (1 << 1) // 是否脏

// 磁盘上的目录项
struct easy_dirent
{
    uint32 d_ino;
    enum easy_file_type d_type;
    char d_name[DIR_MAXLEN];
};

// 内存上的 dentry 缓存
struct easy_dentry
{
    struct easy_dirent d_dd;

    // sleeplock_t lock;

    struct easy_m_inode *d_inode; // 读入内存后首次在哈希表中查找到 inode 后便记录指针，后面就不需要查找了
    struct easy_dentry *d_parent; // 父目录
    struct list_head d_child;     // 指向其中一个子目录（根据 sibling 方可遍历）
    struct list_head d_sibling;   // 兄弟目录链表

    hash_node_t d_hnode;     // 按照名字解析后，以名字哈希到超级块哈希表，便于快速查找
    struct list_head d_list; // 链接到超级块的全局链表中
    struct list_head d_dirty;

    spinlock_t d_lock;
    sleeplock_t d_slock;

    flags_t d_flags;
    atomic_t refcnt;
};

// 1. fs
int efs_mount(struct block_device *bd);
int efs_unmount();

// 2. super block
extern void efs_sb_init();

// extern int efs_sb_fill();
// extern int efs_sb_read();
// extern int efs_sb_write();

extern int efs_imap_alloc();
extern int efs_bmap_alloc();
extern void efs_imap_free(int ino);
extern void efs_bmap_free(int bno);

extern void efs_mount_root();
extern void efs_sb_info();

// 3. inode
// extern void efs_i_fill(struct easy_m_inode *m_inode, int ino);
// extern struct easy_m_inode * efs_i_alloc(int anonymous);
// extern void efs_i_del(struct easy_m_inode *inode);
extern void efs_i_put(struct easy_m_inode *m_inode);
// extern void efs_i_update(struct easy_m_inode *inode);
extern struct easy_m_inode *efs_i_get(int ino);
extern struct easy_m_inode *efs_i_new();
extern int efs_i_unlink(struct easy_m_inode *inode);

// extern void efs_i_sdirty(struct easy_m_inode *inode);
extern void efs_i_cdirty(struct easy_m_inode *inode);
extern void efs_i_type(struct easy_m_inode *inode, enum easy_file_type ftype);

extern int efs_i_read(struct easy_m_inode *inode, uint32 offset, uint32 len, void *vaddr);
extern int efs_i_write(struct easy_m_inode *inode, uint32 offset, uint32 len, void *vaddr);

extern int efs_i_size(struct easy_m_inode *inode);

extern __attribute__((unused)) void efs_i_info(const struct easy_m_inode *inode);

extern void efs_i_root_init();

// 4. dentry
extern void efs_d_lookup(struct easy_dentry *pd);
extern struct easy_dentry *efs_d_creat(struct easy_dentry *pd, const char *name, enum easy_file_type type);
extern void efs_d_unlink(struct easy_dentry *d);
extern struct easy_m_inode *efs_d_namei(const char *path);
extern struct easy_dentry *efs_d_named(const char *path);
extern void efs_d_rename(struct easy_dentry *d, const char *rename);
extern int efs_d_fsize(struct easy_dentry *d);

extern void efs_d_root_init();

extern void efs_d_info(struct easy_dentry *d);
extern void efs_d_infos(struct easy_dentry *d);

extern int efs_d_read(struct easy_dentry *d, uint32 offset, uint32 len, void *vaddr);
extern int efs_d_read_name(const char *path, uint32 offset, uint32 len, void *vaddr);
extern int efs_d_write(struct easy_dentry *d, uint32 offset, uint32 len, void *vaddr);
extern int efs_d_write_name(const char *path, uint32 offset, uint32 len, void *vaddr);

extern struct block_device *efs_bd;

extern struct easy_m_super_block m_esb;

extern struct easy_m_inode root_m_inode;

extern struct easy_dentry root_dentry;

#endif