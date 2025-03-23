// #include "dev/blk/blk_dev.h"
// #include "std/stdio.h"
// #include "fs/bio.h"
// #include "lib/string.h"
// #include "mm/mm.h"


// // 假设为 100M 的话
// #define DISK_SIZE 100 * 1024 * 1024

// static int my_open(struct block_device *bd, mode_t mode);
// static int my_release(struct block_device *bd, mode_t mode);
// // static int my_start_io(struct block_device *bd);
// static uint64 my_disk_size(struct block_device *bd);
// static int my_ll_rw(struct block_device *bd, struct bio *bio, uint32 rw);
// // static int my_read(struct block_device *bd, uint64 blockno, uint32 count, char *vaddr);
// // static int my_write(struct block_device *bd, uint64 blockno, uint32 count, char *vaddr);

// // 暴露下方便测试
// struct block_device my_dev;
// // static struct block_device my_dev;
// static struct block_device_operations bd_ops = {
//     .open = my_open,
//     .release = my_release,
//     // .start_io = my_start_io,
//     .disk_size = my_disk_size,
//     .ll_rw = my_ll_rw,
//     // .read = my_read,
//     // .write = my_write,
// };

// void my_dev_init()
// {
//     my_dev.bdev_ops = &bd_ops;
//     my_dev.disk_size = DISK_SIZE;
//     strncpy(my_dev.name, "my_dev", sizeof(my_dev.name));
//     register_block(&my_dev);
// }

// static int my_open(struct block_device *bd, mode_t mode)
// {
//     printk("my_open\n");
//     return 0;
// }
// static int my_release(struct block_device *bd, mode_t mode)
// {
//     printk("my_release\n");
//     return 0;
// }
// // static int my_start_io(struct block_device *bd)
// // {
// //     printk("my_start_io\n");
// //     return 0;
// // }
// static inline uint64 my_disk_size(struct block_device *bd)
// {
//     printk("my_disk_size\n");
//     return bd->disk_size;
// }
// static int my_ll_rw(struct block_device *bd, struct bio *bio, uint32 rw)
// {
//     printk("my_ll_rw\n");
//     return 0;
// }
// // static int my_read(struct block_device *bd, uint64 blockno, uint32 count, char *vaddr)
// // {
// //     printk("my_read\n");
// //     return 0;
// // }
// // static int my_write(struct block_device *bd, uint64 blockno, uint32 count, char *vaddr)
// // {
// //     printk("my_write\n");
// //     return 0;
// // }