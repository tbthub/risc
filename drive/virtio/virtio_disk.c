//
// driver for qemu's virtio disk device.
// uses qemu's mmio interface to virtio.
//
// qemu ... -drive file=fs.img,if=none,format=raw,id=x0 -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0
//

#include "std/stddef.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "mm/memlayout.h"
#include "lib/spinlock.h"
#include "lib/sleeplock.h"
#include "dev/blk/buf.h"
#include "virtio.h"
#include "mm/kmalloc.h"
#include "dev/blk/blk_dev.h"
#include "dev/devs.h"
#include "lib/string.h"

#define DISK_SIZE 100 * 1024 * 1024
#define SECTOR_SIZE 512
#define DISK_NAME "virtio_disk"

// the address of virtio mmio register r.
#define R(r) ((volatile uint32 *)(VIRTIO0 + (r)))

static struct disk
{
    // DMA描述符 (struct virtq_desc *desc)
    // 用途：desc 是一个DMA描述符的数组，描述符用于告诉设备在磁盘上进行读写操作的具体位置。
    // 描述符表示单个 I/O 操作的元数据，例如数据的位置和长度。
    // 数量：有 NUM 个描述符，每个描述符可以链接起来形成一个链表，用于描述一个完整的磁盘操作命令。
    struct virtq_desc *desc;

    // 可用环 (struct virtq_avail *avail)
    // 用途：avail 是一个环形队列，驱动程序将准备好处理的描述符编号写入该队列。
    // 这里每次只包含链表的头描述符编号，即代表一个操作的开始。
    // 描述：设备将会读取 avail 环中的描述符编号，知道驱动程序希望它处理哪些描述符链。
    struct virtq_avail *avail;

    // 已使用环 (struct virtq_used *used)
    // 用途：used 也是一个环形队列，设备会将处理完的描述符编号写回这个队列。
    // 设备只会写入链表的头描述符编号，表示该链表（即该操作）已经完成。
    // 数量：used 环中最多有 NUM 个已使用的描述符条目。
    struct virtq_used *used;

    // 记录每个描述符是否空闲
    char free[NUM]; // is a descriptor free?
    sleeplock_t free_0;
    // 用于跟踪 used 环的读取进度
    uint16 used_idx; // we've looked this far in used[2..NUM].

    // 用于记录正在进行中的磁盘操作的信息。数组索引对应于每个描述符链的头描述符编号
    struct
    {
        struct buf *b;
        char status;
    } info[NUM];

    // 存储磁盘操作命令头的数组
    struct virtio_blk_req ops[NUM];

    sleeplock_t intr;
    mutex_t mutex;

} disk;

static int virtio_disk_ll_rw(struct gendisk *gd, struct bio *bio, uint32 rw);
static struct gendisk_operations virtio_disk_ops = {
    .ll_rw = virtio_disk_ll_rw,
};
struct block_device virtio_disk;

void virtio_disk_init(void)
{
    uint32 status = 0;
    sleep_init_zero(&disk.intr, "");
    mutex_init(&disk.mutex, "virtio_mutex");

    if (*R(VIRTIO_MMIO_MAGIC_VALUE) != 0x74726976 ||
        *R(VIRTIO_MMIO_VERSION) != 2 ||
        *R(VIRTIO_MMIO_DEVICE_ID) != 2 ||
        *R(VIRTIO_MMIO_VENDOR_ID) != 0x554d4551)
    {
        panic("could not find virtio disk");
    }

    // reset device
    *R(VIRTIO_MMIO_STATUS) = status;

    // set ACKNOWLEDGE status bit
    status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;
    *R(VIRTIO_MMIO_STATUS) = status;

    // set DRIVER status bit
    status |= VIRTIO_CONFIG_S_DRIVER;
    *R(VIRTIO_MMIO_STATUS) = status;

    // negotiate features
    uint64 features = *R(VIRTIO_MMIO_DEVICE_FEATURES);
    features &= ~(1 << VIRTIO_BLK_F_RO);
    features &= ~(1 << VIRTIO_BLK_F_SCSI);
    features &= ~(1 << VIRTIO_BLK_F_CONFIG_WCE);
    features &= ~(1 << VIRTIO_BLK_F_MQ);
    features &= ~(1 << VIRTIO_F_ANY_LAYOUT);
    features &= ~(1 << VIRTIO_RING_F_EVENT_IDX);
    features &= ~(1 << VIRTIO_RING_F_INDIRECT_DESC);
    *R(VIRTIO_MMIO_DRIVER_FEATURES) = features;

    // tell device that feature negotiation is complete.
    status |= VIRTIO_CONFIG_S_FEATURES_OK;
    *R(VIRTIO_MMIO_STATUS) = status;

    // re-read status to ensure FEATURES_OK is set.
    status = *R(VIRTIO_MMIO_STATUS);
    if (!(status & VIRTIO_CONFIG_S_FEATURES_OK))
        panic("virtio disk FEATURES_OK unset");

    // initialize queue 0.
    *R(VIRTIO_MMIO_QUEUE_SEL) = 0;

    // ensure queue 0 is not in use.
    if (*R(VIRTIO_MMIO_QUEUE_READY))
        panic("virtio disk should not be ready");

    // check maximum queue size.
    uint32 max = *R(VIRTIO_MMIO_QUEUE_NUM_MAX);
    if (max == 0)
        panic("virtio disk has no queue 0");
    if (max < NUM)
        panic("virtio disk max queue too short");

    // allocate and zero queue memory.
    disk.desc = kmalloc(sizeof(struct virtq_desc), 0);
    disk.avail = kmalloc(sizeof(struct virtq_avail), 0);
    disk.used = kmalloc(sizeof(struct virtq_used), 0);
    if (!disk.desc || !disk.avail || !disk.used)
        panic("virtio disk kalloc");
    memset(disk.desc, 0, sizeof(struct virtq_desc));
    memset(disk.avail, 0, sizeof(struct virtq_avail));
    memset(disk.used, 0, sizeof(struct virtq_used));

    // set queue size.
    *R(VIRTIO_MMIO_QUEUE_NUM) = NUM;

    // write physical addresses.
    *R(VIRTIO_MMIO_QUEUE_DESC_LOW) = (uint64)disk.desc;
    *R(VIRTIO_MMIO_QUEUE_DESC_HIGH) = (uint64)disk.desc >> 32;
    *R(VIRTIO_MMIO_DRIVER_DESC_LOW) = (uint64)disk.avail;
    *R(VIRTIO_MMIO_DRIVER_DESC_HIGH) = (uint64)disk.avail >> 32;
    *R(VIRTIO_MMIO_DEVICE_DESC_LOW) = (uint64)disk.used;
    *R(VIRTIO_MMIO_DEVICE_DESC_HIGH) = (uint64)disk.used >> 32;

    // queue is ready.
    *R(VIRTIO_MMIO_QUEUE_READY) = 0x1;

    // all NUM descriptors start out unused.
    for (int i = 0; i < NUM; i++)
        disk.free[i] = 1;

    // tell device we're completely ready.
    status |= VIRTIO_CONFIG_S_DRIVER_OK;
    *R(VIRTIO_MMIO_STATUS) = status;

    register_block(&virtio_disk, &virtio_disk_ops, DISK_NAME, DISK_SIZE);
}

// find a free descriptor, mark it non-free, return its index.
static int
alloc_desc()
{
    for (int i = 0; i < NUM; i++)
    {
        if (disk.free[i])
        {
            disk.free[i] = 0;
            return i;
        }
    }
    return -1;
}

// mark a descriptor as free.
static void
free_desc(int i)
{
    if (i >= NUM)
        panic("free_desc 1");
    if (disk.free[i])
        panic("free_desc 2");
    disk.desc[i].addr = 0;
    disk.desc[i].len = 0;
    disk.desc[i].flags = 0;
    disk.desc[i].next = 0;
    disk.free[i] = 1;
}

// free a chain of descriptors.
static void
free_chain(int i)
{
    while (1)
    {
        int flag = disk.desc[i].flags;
        int nxt = disk.desc[i].next;
        free_desc(i);
        if (flag & VRING_DESC_F_NEXT)
            i = nxt;
        else
            break;
    }
}

// allocate three descriptors (they need not be contiguous).
// disk transfers always use three descriptors.
static int
alloc3_desc(int *idx)
{
    for (int i = 0; i < 3; i++)
    {
        idx[i] = alloc_desc();
        if (idx[i] < 0)
        {
            for (int j = 0; j < i; j++)
                free_desc(idx[j]);
            return -1;
        }
    }
    return 0;
}

static void virtio_disk_rw(struct bio *bio, int rw)
{
    uint64 sector = bio->b_blockno * (PGSIZE / SECTOR_SIZE);
    mutex_lock(&disk.mutex);
    // the spec's Section 5.2 says that legacy block operations use
    // three descriptors: one for type/reserved/sector, one for the
    // data, one for a 1-byte status result.

    // allocate the three descriptors.
    int idx[3];
    alloc3_desc(idx);

    // format the three descriptors.
    // qemu's virtio-blk.c reads them.

    struct virtio_blk_req *buf0 = &disk.ops[idx[0]];

    if (rw == DEV_WRITE)
        buf0->type = VIRTIO_BLK_T_OUT; // write the disk
    else
        buf0->type = VIRTIO_BLK_T_IN; // read the disk
    buf0->reserved = 0;
    buf0->sector = sector;

    disk.desc[idx[0]].addr = (uint64)buf0;
    disk.desc[idx[0]].len = sizeof(struct virtio_blk_req);
    disk.desc[idx[0]].flags = VRING_DESC_F_NEXT;
    disk.desc[idx[0]].next = idx[1];

    disk.desc[idx[1]].addr = (uint64)bio->b_page;
    disk.desc[idx[1]].len = PGSIZE;
    if (rw == DEV_WRITE)
        disk.desc[idx[1]].flags = 0; // device reads bio->b_page
    else
        disk.desc[idx[1]].flags = VRING_DESC_F_WRITE; // device writes b->data
    disk.desc[idx[1]].flags |= VRING_DESC_F_NEXT;
    disk.desc[idx[1]].next = idx[2];

    disk.info[idx[0]].status = 0xff; // device writes 0 on success
    disk.desc[idx[2]].addr = (uint64)&disk.info[idx[0]].status;
    disk.desc[idx[2]].len = 1;
    disk.desc[idx[2]].flags = VRING_DESC_F_WRITE; // device writes the status
    disk.desc[idx[2]].next = 0;

    // record struct buf for virtio_disk_intr().

    // tell the device the first index in our chain of descriptors.
    disk.avail->ring[disk.avail->idx % NUM] = idx[0];

    __sync_synchronize();

    // tell the device another avail ring entry is available.
    disk.avail->idx += 1; // not % NUM ...

    __sync_synchronize();

    *R(VIRTIO_MMIO_QUEUE_NOTIFY) = 0; // value is queue number
    sleep_on(&disk.intr);

    free_chain(idx[0]);
    mutex_unlock(&disk.mutex);
}

void virtio_disk_intr()
{
    // 由于上面在互斥睡眠，这里可以直接动，不必申请互斥锁
    *R(VIRTIO_MMIO_INTERRUPT_ACK) = *R(VIRTIO_MMIO_INTERRUPT_STATUS) & 0x3;

    __sync_synchronize();

    // the device increments disk.used->idx when it
    // adds an entry to the used ring.

    while (disk.used_idx != disk.used->idx)
    {
        __sync_synchronize();
        int id = disk.used->ring[disk.used_idx % NUM].id;

        if (disk.info[id].status != 0)
            panic("virtio_disk_intr status");

        wake_up(&disk.intr);

        disk.used_idx += 1;
    }
}

static int virtio_disk_ll_rw(struct gendisk *gd, struct bio *bio, uint32 rw)
{
    virtio_disk_rw(bio, rw);
    return 0;
}
