#include "core/proc.h"
#include "mm/mm.h"
#include "dev/blk/blk_dev.h"
#include "core/timer.h"
#include "lib/string.h"

extern struct block_device my_dev;
extern struct block_device mvirt_blk_dev;
extern struct block_device virtio_disk;
static void blk_test()
{
    // printk("blk_test\n");
    // char *addr = __alloc_page(0);
    // blk_read(&virtio_disk, 100, 0, 4096, addr);
    // blk_read(&virtio_disk, 100, 0, 4096, addr);
    // memset(addr + 2, 'F', 10);
    // // addr[1] = 'S';
    // blk_write(&virtio_disk, 100, 0, 4096, addr);
    // memset(addr, '\0', 100);
    // blk_read(&virtio_disk, 100, 0, 4096, addr);
    // addr[1] = 'B';
    // blk_write(&virtio_disk, 100, 0, 1000, addr);
    // char test_str[50];
    // memset(test_str,'\0',sizeof(test_str));
    // strdup(test_str, "hello, this is a test!");
    // blk_write(&virtio_disk, 1, 16, 50, test_str);
    // blk_write(&virtio_disk, 2, 10, 50, test_str);
    // blk_write(&virtio_disk, 4, 4092, 50, test_str);

    // void *vaddr = __alloc_page(0);
    // memset(vaddr,'v',PGSIZE);
    // blk_write(&virtio_disk, 5, 0, PGSIZE, vaddr);

    // char test_str2[50];
    // blk_read(&virtio_disk, 1, 18, 50, test_str2);
    // printk("%s\n",test_str2);

    // virtio_disk.gd.ops.write(&virtio_disk.gd, 1, 1, vaddr);

    // memset(vaddr + PGSIZE, 'b', PGSIZE);
    // virtio_disk.gd.ops.write(&virtio_disk.gd, 2, 1, vaddr + PGSIZE);

    // virtio_disk.gd.ops.read(&virtio_disk.gd, 1, 2, vaddr);
    // int count = 0;
    // while (vaddr[count++])
    //     ;
    // // printk("b_test: %p\n",vaddr);

    // printk("read from blockno: 1, count: %d\n", count);

    // printk("%s\n", vaddr);
    // printk("virtio_disk disk_size: %p\n",virtio_disk.gd.ops.disk_size(&virtio_disk.gd));
}

void block_func_test()
{
    kthread_create(blk_test, NULL, "blk_test",NO_CPU_AFF);
}
