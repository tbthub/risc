#ifndef __BLK_DEV_H__
#define __BLK_DEV_H__
#include "std/stddef.h"

#include "lib/list.h"
#include "lib/hash.h"
#include "lib/atomic.h"
#include "core/locks/sleeplock.h"
#include "core/locks/spinlock.h"

#include "std/stddef.h"

#include "dev/blk/gendisk.h"

struct block_device
{
    dev_t bd_dev;             // 设备号
    struct list_head bd_list; // 设备链
    char name[16];

    uint64_t disk_size; // 设备大小
    struct gendisk gd;
    void *private; // 每个结构特定的私有域
};

extern int register_block(struct block_device *bd, const struct gendisk_operations *ops, const char *name, uint64_t disk_size);
extern int unregister_block(struct block_device *bd);
extern void blk_set_private(struct block_device *bd, void *private);

extern int blk_read(struct block_device *bd, uint32_t blockno, uint32_t offset, uint32_t len, void *vaddr);
extern int blk_write(struct block_device *bd, uint32_t blockno, uint32_t offset, uint32_t len, void *vaddr);

extern int blk_read_count(struct block_device *bd, uint32_t blockno, uint32_t count, void *vaddr);
extern int blk_write_count(struct block_device *bd, uint32_t blockno, uint32_t count, void *vaddr);
#endif
