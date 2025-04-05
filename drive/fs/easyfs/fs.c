#include "easyfs.h"
#include "fs/fs.h"
#include "dev/blk/blk_dev.h"


struct block_device *efs_bd;

int efs_mount(struct block_device *bd)
{
    printk("easy fs init start...\n");
    efs_bd = bd;

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

    return 0;
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
