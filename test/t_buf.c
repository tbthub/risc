// #include "dev/blk/buf.h"

// static void buf_debug(struct buf_head *buf)
// {
//     printk("buf: %p\n", buf);
//     printk("buf->flags: %d\n", buf->flags);
//     printk("buf->refcnt: %d\n", buf->refcnt);
//     printk("buf->data: %p\n", buf->data);
//     if (buf->gd)
//         printk("buf->b_dev: %p\n", buf->gd->dev->name);
// }

// void buf_test()
// {
//     struct buf *buf1 = bget(NULL, 0);
//     struct buf *buf2 = bget(NULL, 0);
//     if (buf1 != buf2)
//     {
//         panic("buf1!=buf2\n");
//     }

//     buf_debug(buf1);
//     buf_debug(buf2);
    
//     brelse(buf1);
//     // buf_debug(buf2);
//     buf_pin(buf2);
//     brelse(buf2);
//     buf_debug(buf2);
    
//     buf_pin(buf2);
//     brelse(buf2);
//     buf_debug(buf2);
// }