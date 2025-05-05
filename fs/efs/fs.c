#include "core/module.h"
#include "dev/blk/blk_dev.h"
#include "efs.h"
#include "fs/fcntl.h"
#include "mm/slab.h"

struct block_device *efs_bd;
extern struct block_device virtio_disk;
extern int8_t efs_open(vfs_file_context_t *context, uint8_t *path, int flags, int mode);
extern int8_t efs_close(vfs_file_context_t *context);
extern int32_t efs_read(vfs_file_context_t *context, uint8_t *buffer, size_t size);
extern int32_t efs_write(vfs_file_context_t *context, uint8_t *buffer, size_t size);
extern int32_t efs_lseek(vfs_file_context_t *context, int32_t offset, int whence);
extern int8_t efs_dup2(vfs_file_context_t *old_file, vfs_file_context_t *new_file);

struct kmem_cache efs_inode_kc;
struct kmem_cache efs_dentry_kc;
struct kmem_cache efs_file_kc;

static void *efs_mount(void *arg)
{
    assert(efs_bd == NULL, "efs_mount");
    efs_bd = (struct block_device *)arg;

    // 1. sb
    efs_sb_init();

    // 2. root inode
    efs_i_root_init();

    // 3. root dentry
    efs_d_root_init();

#ifdef DEBUG_FS
    efs_sb_info();
    efs_i_info(&root_m_inode);
#endif
    return (void *)1;
}

static int8_t efs_unmount()
{
    efs_bd = NULL;
    printk("efs_unmount\n");
    return 0;
}

static void efs_io_table_init(vfs_io_t *table)
{
    table->open = efs_open;
    table->read = efs_read;
    table->write = efs_write;
    table->close = efs_close;
    table->mount = efs_mount;
    table->umount = efs_unmount;
    table->lseek = efs_lseek;
}

static int efs_init()
{
    printk("efs_init start\n");

    // 挂载函数里面有使用到缓存
    kmem_cache_create(&efs_inode_kc, "inode_kc", sizeof(struct easy_m_inode), 0);
    kmem_cache_create(&efs_dentry_kc, "dentry_kc", sizeof(struct easy_dentry), 0);
    kmem_cache_create(&efs_file_kc, "efs_file_kc", sizeof(struct efs_file), 0);
    
    vfs_io_t efs_table;
    efs_table.mountTag = "efs";
    efs_io_table_init(&efs_table);
    assert(vfs_mount(&efs_table, &virtio_disk) >= 0, "efs_mount");
    
    printk("efs_init done\n");
    return 0;
}

static void efs_exit()
{
    printk("efs_exit\n");
}

module_init_level(efs_init, INIT_LEVEL_FS);
module_exit_level(efs_exit, INIT_LEVEL_FS);
