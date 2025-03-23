#include "dev/blk/gendisk.h"
#include "lib/string.h"
#include "dev/devs.h"
#define BLK_SIZE 4096

inline int blk_read(struct block_device *bd, uint32 blockno, uint32 offset, uint32 len, void *vaddr)
{
    return gen_disk_read(&bd->gd, blockno, offset, len, vaddr);
}

inline int blk_write(struct block_device *bd, uint32 blockno, uint32 offset, uint32 len, void *vaddr)
{
    return gen_disk_write(&bd->gd, blockno, offset, len, vaddr);
}

inline int blk_write_count(struct block_device *bd, uint32 blockno, uint32 count, void *vaddr)
{
    return gen_disk_write(&bd->gd, blockno, 0, count * BLK_SIZE, vaddr);
}

inline int blk_read_count(struct block_device *bd, uint32 blockno, uint32 count, void *vaddr)
{
    return gen_disk_read(&bd->gd, blockno, 0, count * BLK_SIZE, vaddr);
}

inline void blk_set_private(struct block_device *bd, void *private)
{
    bd->private = private;
}

int register_block(struct block_device *bd, const struct gendisk_operations *ops, const char *name, uint64 disk_size)
{
    bd->bd_dev = devno_alloc(); // 分配设备号
    devs_add(&bd->bd_list);     // 加入全局链表
    strncpy(bd->name, name, 16);
    bd->disk_size = disk_size;
    gendisk_init(bd, ops);
    return 0;
}

int unregister_block(struct block_device *bd)
{
    // 暂未实现
    return 0;
}
