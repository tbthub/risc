#ifndef __REQUEST_H__
#define __REQUEST_H__

#include "core/locks/semaphore.h"
#include "core/locks/sleeplock.h"
#include "dev/blk/bio.h"
#include "lib/list.h"
#include "std/stddef.h"

#define REQUEST_READ 0
#define REQUEST_WRITE 1
#define REQUEST_NONE 2


struct gendisk;
// 一次请求创建一个 request,当请求的块数大于一个页面时候，会含有多个bio,
struct request
{
    spinlock_t lock;
    struct list_head queue_node;
    // 本次请求是读操作还是写操作？
    uint32 rq_flags;
    // 请求的 bio 链（不含链表头）
    struct bio *bio;
    uint8 completed;

    // 这个锁只是用于同步的，并不实现互斥。为什么不直接用信号量呢？
    // 因为“睡眠在这个请求上”、“唤醒等待这个请求完成的线程比较好听”
    sleeplock_t slock;

    void *vaddr;
};

struct request_queue
{
    spinlock_t lock;  // 加锁控制插入、删除队列
    semaphore_t sem;  // 有 request 加入就唤醒，用于同步
    struct gendisk *gd;
    struct list_head rq_list;
};

extern void rq_queue_init(struct gendisk *gd);
extern struct request *make_request(struct gendisk *gd, uint64 blockno, uint32 offset, uint32 len, void *vaddr, uint32 rw);
extern struct request *get_next_rq(struct request_queue *rq_queue);
extern void rq_del(struct request *rq);

extern void rq_set_complete(struct request *rq);
extern int rq_test_complete(struct request *rq);
extern void rq_clear_complete(struct request *rq);

extern void sleep_on_rq(struct request *rq);
extern void wake_up_rq(struct request *rq);

#endif
