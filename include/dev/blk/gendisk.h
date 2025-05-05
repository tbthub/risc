
#ifndef __GEN_DISK_H__
#define __GEN_DISK_H__

#include "std/stddef.h"

#include "dev/blk/bio.h"
#include "dev/blk/request.h"
#include "dev/blk/buf.h"

struct gendisk_operations
{
    int (*open)(struct gendisk *gd, mode_t mode);    // 打开设备
    int (*release)(struct gendisk *gd, mode_t mode); // 关闭设备
    int (*start_io)(struct gendisk *gd);             // 执行I/O操作，由一个专门线程负责
    uint64_t (*disk_size)(struct gendisk *gd);         // 获取设备大小

    int (*ll_rw)(struct gendisk *gd, struct bio *bio, uint32_t rw); // 设备底层次的读写操作

    // 读写操作，只是创建对应的 bio 后挂载到请求队列上
    int (*read)(struct gendisk *gd, uint32_t blockno, uint32_t offset, uint32_t len, void *vaddr);
    int (*write)(struct gendisk *gd, uint32_t blockno, uint32_t offset, uint32_t len, void *vaddr);
};

// 通用块
struct gendisk
{
    struct block_device *dev;
    struct request_queue queue; // 请求队列

    struct gendisk_operations ops;

    struct bhash_struct bhash;        // 缓存哈希
    struct thread_info *io_thread;    // 专门负责处理这个设备IO的线程
    struct thread_info *flush_thread; // 专门负责处理这个设备 flush 的线程
};

extern void gendisk_init(struct block_device *bd, const struct gendisk_operations *ops);
extern int gen_disk_read(struct gendisk *gd, uint32_t blockno, uint32_t offset, uint32_t len, void *vaddr);
extern int gen_disk_write(struct gendisk *gd, uint32_t blockno, uint32_t offset, uint32_t len, void *vaddr);

#endif
