
// 一个在内存中存在的，虚拟的块设备
#include "dev/blk/blk_dev.h"
#include "std/stdio.h"
#include "dev/blk/bio.h"
#include "std/string.h"
#include "mm/mm.h"
#include "dev/devs.h"
#include "dev/blk/gendisk.h"
#include "riscv.h"

// 假设为 4M,起始也就是最大的那个伙伴块，每个物理块直接按 512算，文件系统块是 4K
#define DISK_SIZE 4 * 1024 * 1024
#define SECTOR_SIZE 512
#define order 10

// 作为磁盘的起始位置
static void *addr;

static int mvirt_blk_open(struct gendisk *gd, mode_t mode);
static int mvirt_blk_release(struct gendisk *gd, mode_t mode);
static int mvirt_blk_ll_rw(struct gendisk *gd, struct bio *bio, uint32 rw);

struct block_device mvirt_blk_dev;
static struct gendisk_operations gd_ops = {
    .open    = mvirt_blk_open,
    .release = mvirt_blk_release,
    .ll_rw   = mvirt_blk_ll_rw,
};

void mvirt_blk_dev_init() {
    addr = __alloc_pages(0, order);
    memset(addr, 'a', DISK_SIZE);
    blk_set_private(&mvirt_blk_dev, addr);
    register_block(&mvirt_blk_dev, &gd_ops, "mvirt_blk_dev", DISK_SIZE);
}

static int mvirt_blk_open(struct gendisk *gd, mode_t mode) {
    printk("mvirt_blk_open\n");
    return 0;
}

static int mvirt_blk_release(struct gendisk *gd, mode_t mode) {
    printk("mvirt_blk_release\n");
    return 0;
}

static int mvirt_blk_ll_rw(struct gendisk *gd, struct bio *bio, uint32 rw) {
    printk("mvirt_blk_ll_rw  rw: %d\n", rw);
    // 起始位置
    uint64 start = bio->b_blockno * PGSIZE;

    if (rw == DEV_READ)
        memcpy(bio->b_page, addr + start, PGSIZE);
    else if (rw == DEV_WRITE)
        memcpy(addr + start, bio->b_page, PGSIZE);

    return 0;
}
