#include "dev/blk/request.h"

#include "core/locks/semaphore.h"
#include "dev/blk/bio.h"
#include "dev/blk/gendisk.h"
#include "mm/kmalloc.h"

inline void rq_set_complete(struct request *rq)
{
    rq->completed = 1;
}

inline int rq_test_complete(struct request *rq)
{
    return rq->completed == 1;
}

inline void rq_clear_complete(struct request *rq)
{
    rq->completed = 0;
}

// 申请空白的一个 request
static struct request *request_alloc()
{
    struct request *rq = kmalloc(sizeof(struct request), 0);
    if (!rq)
        return NULL;
    INIT_LIST_HEAD(&rq->queue_node);
    rq->rq_flags = 0;
    rq->bio = NULL;
    rq->completed = 0;
    spin_init(&rq->lock, "rq");
    sleep_init_zero(&rq->slock, "rq-sleep");
    rq->vaddr = NULL;
    return rq;
}

// 将 request 插入指定的 queue
static inline void rq_append(struct request *rq, struct request_queue *rq_queue)
{
    spin_lock(&rq_queue->lock);
    list_add_tail(&rq->queue_node, &rq_queue->rq_list);
    spin_unlock(&rq_queue->lock);
}

struct request *make_request(struct gendisk *gd, uint64 blockno, uint32 offset, uint32 len, void *vaddr, uint32 rw)
{
#ifdef DEBUG_RQ
    printk("rq: devno: %d, bno:%d,offset:%d, len:%d, vaddr:%p, rw:%d\n", 0, blockno, offset, len, vaddr, rw);
#endif
    struct request *rq = request_alloc();
    if (!rq)
        panic("make_request request_alloc!\n");

    rq->rq_flags = rw;
    rq->bio = bio_list_make(blockno, offset, len);
#ifdef DEBUG_BIO
    struct bio *bi = rq->bio;
    while (bi) {
        printk("bio_alloc: no,off,len: %d, %d~%d \ts: %d -> e: %d\n", bi->b_blockno, bi->offset, bi->len, bi->offset, bi->offset + bi->len);
        bi = bi->b_next;
    }

#endif
    rq->vaddr = vaddr;
    rq_append(rq, &gd->queue);
    return rq;
}

void rq_del(struct request *rq)
{
    kfree(rq);
}

// 弹出下一个请求（会从队列中移除）
struct request *get_next_rq(struct request_queue *rq_queue)
{
    struct request *next = NULL;
    spin_lock(&rq_queue->lock);
    if (!list_empty(&rq_queue->rq_list))
        next = list_entry(list_pop(&rq_queue->rq_list), struct request, queue_node);

    spin_unlock(&rq_queue->lock);
    return next;
}

// 初始化该设备的请求队列
void rq_queue_init(struct gendisk *gd)
{
    struct request_queue *rq_queue = &gd->queue;
    spin_init(&rq_queue->lock, "dev_rq_queue");
    sem_init(&gd->queue.sem, 0, "dev_rq_sem");  // 初始信号量为 0
    rq_queue->gd = gd;
    INIT_LIST_HEAD(&rq_queue->rq_list);
}

inline void sleep_on_rq(struct request *rq)
{
    if (!sleep_is_hold(&rq->slock))
        sleep_on(&rq->slock);
    else
        panic("sleep_on_rq\n");
}

inline void wake_up_rq(struct request *rq)
{
    wake_up(&rq->slock);
}
