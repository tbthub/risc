#include "easyfs.h"
#include "dev/blk/blk_dev.h"
#include "fs/fcntl.h"


struct block_device *efs_bd;

extern int8_t efs_open(vfs_file_context_t *context, uint8_t *path, int flags, int mode);
extern int8_t efs_close(vfs_file_context_t *context);
extern int32_t efs_read(vfs_file_context_t *context, uint8_t *buffer, size_t size);
extern int32_t efs_write(vfs_file_context_t *context, uint8_t *buffer, size_t size);
extern int32_t efs_lseek(vfs_file_context_t *context, int32_t offset, int whence);
extern int8_t efs_dup2(vfs_file_context_t *old_file, vfs_file_context_t *new_file);


int efs_mount(struct vfs_io_t * vio)
{
    printk("easy fs init start...\n");
    efs_bd = (struct block_device *)vio->io_args;

    // 1. sb
    efs_sb_init();

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

    printk("easy fs init done.\n");
    return 0;
}

int efs_unmount()
{
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
    table->dup2   = efs_dup2;
    table->mountTag = "efs";
}
