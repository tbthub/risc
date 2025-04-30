#include "easyfs.h"
#include "dev/blk/blk_dev.h"
#include "fs/fcntl.h"
#include "file.h"


struct block_device *efs_bd;

extern int8_t efs_open(vfs_file_context_t *context, uint8_t *path, int flags, int mode);
extern int8_t efs_close(vfs_file_context_t *context);
extern int32_t efs_read(vfs_file_context_t *context, uint8_t *buffer, size_t size);
extern int32_t efs_write(vfs_file_context_t *context, uint8_t *buffer, size_t size);
extern int32_t efs_lseek(vfs_file_context_t *context, int32_t offset, int whence);
extern int8_t efs_dup2(vfs_file_context_t *old_file, vfs_file_context_t *new_file);

void *efs_mount(void *arg) {
    printk("easy fs init start...\n");
    assert(efs_bd == NULL, "efs_mount");
    efs_bd = (struct block_device *)arg;

    kmem_cache_create(&efs_inode_kmem_cache, "inode_kmem_cache", sizeof(struct easy_m_inode), 0);
    kmem_cache_create(&efs_dentry_kmem_cache, "dentry_kmem_cache", sizeof(struct easy_dentry), 0);
    kmem_cache_create(&file_kmem_cache, "file_kmem_cache", sizeof(struct file), 0);

    printk("1\n");
    // 1. sb
    efs_sb_init();
    printk("2\n");
    // 2. root inode
    efs_i_root_init();
    
    // 3. root dentry
    efs_d_root_init();
    
    printk("easy fs init done\n");


#ifdef DEBUG_FS
    efs_sb_info();
    efs_i_info(&root_m_inode);
#endif

    // 3. root dentry
    // efs_rootd_obtain();

    // efs_info();

    // 4. mount root
    // efs_mount_root(&root_m_inode, &root_dentry);

    // return 0;
    // 2. 注册 VFS
    // 3. 挂载

    return (void*)1;
}

int8_t efs_unmount()
{
    efs_bd = NULL;
    printk("efs_unmount\n");
    return 0;
}

void efs_io_table_init(vfs_io_t *table) {
    table->open   = efs_open;
    table->read   = efs_read;
    table->write  = efs_write;
    table->close  = efs_close;
    table->mount  = efs_mount;
    table->umount = efs_unmount;
    table->lseek  = efs_lseek;
}
