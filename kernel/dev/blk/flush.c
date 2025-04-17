#include "dev/blk/bio.h"
#include "dev/blk/buf.h"
#include "dev/blk/gendisk.h"
#include "core/locks/semaphore.h"
#include "dev/devs.h"
#include "core/timer.h"
#include "core/proc.h"

// static buf_destory(struct buf_head *buf)
// {

// }

// 暂时没有完全实现
// 1. inactive_list 一半释放
// 2. active_list 一半移入 inactive_list
// 3. dirty_list 同步,
void flush_bhash(void *args)
{
        struct bhash_struct *bhash = (struct bhash_struct *)args;
        struct buf_head *buf;
        struct gendisk *gd = bhash->gd;
        struct bio bio;
        int num_written = 0;
        // printk("%s flush_bhash start timer: 2500\n", bhash->gd->dev->name);
        struct thread_info *p  = myproc();
        while (1)
        {
                thread_timer_sleep(p, 2500);
#ifdef DEBUG_FLUSH
                printk("%s flush on hart: %d\n", bhash->gd->dev->name, cpuid());
#endif

                // 获取自旋锁，同步磁盘
                spin_lock(&bhash->lock);

                // 检查是否有脏数据块
                while (!list_empty(&bhash->dirty_list))
                {
                        buf = list_entry(list_pop(&bhash->dirty_list), struct buf_head, dirty);
                        // 释放自旋锁，避免持有锁时进行阻塞 I/O
                        spin_unlock(&bhash->lock);
                        
                        buf_pin(buf);
                        // 设置 bio 并执行写回
                        bio.b_page = buf->page;
                        bio.b_blockno = buf->blockno;
                        gd->ops.ll_rw(gd, &bio, DEV_WRITE); // 写磁盘                   这里面队列有没有锁
#ifdef DEBUG_FLUSH
                        printk("%d flush blockno: %d to disk on hart %d\n",num_written, bio.b_blockno,cpuid());
#endif

                        buf_unpin(buf);
                        num_written++;

                        // 重新获取自旋锁，处理下一个脏数据块
                        spin_lock(&bhash->lock);
                        CLEAR_FLAG(&buf->flags, BH_Dirty);
                        list_del(&buf->dirty);
                }

                spin_unlock(&bhash->lock);
#ifdef DEBUG_FLUSH
                printk("Number of blocks written back this time: %d\n", num_written);
#endif
                num_written = 0;
        }
}
