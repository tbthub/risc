#include "core/work.h"
#include "lib/semaphore.h"
#include "lib/mutex.h"
#include "lib/list.h"
#include "mm/kmalloc.h"
#include "core/proc.h"
#include "lib/spinlock.h"

// 工作队列机制

struct work_struct
{
    void (*func)(void *);
    void *args;
    struct list_head list;
};

static struct work_queue_struct
{
    struct fifo queue;
    spinlock_t lock; // 保护队列

    semaphore_t count;
} work_queue[NCPU];

static struct work_struct *work_alloc(void (*func)(void *), void *args)
{
    struct work_struct *w = kmalloc(sizeof(struct work_struct), 0);
    if (!w)
    {
        printk("work.c [work_alloc]: work_struct memory allocation failed\n");
        return NULL;
    }
    w->func = func;
    w->args = args;
    INIT_LIST_HEAD(&w->list);
    return w;
}

static struct work_struct *work_queue_pop()
{
    // push_off();
    int cid = cpuid();
    // pop_off();
    // printk("hart %d wait\n", cpuid());
    sem_wait(&work_queue[cid].count);
    // printk("hart %d run\n", cpuid());
    spin_lock(&work_queue[cid].lock);
    struct list_head *node = fifo_pop(&work_queue[cid].queue);
    spin_unlock(&work_queue[cid].lock);
    if (!node)
    {
        return NULL;
    }
    struct work_struct *w = list_entry(node, struct work_struct, list);
    return w;
}

static void work_free(struct work_struct *w)
{
    kfree(w);
}

static __attribute__((noreturn)) void kthread_work_handler()
{
    for (;;)
    {
        struct work_struct *w = work_queue_pop();
        w->func(w->args);
        work_free(w);
    }
}

void work_queue_init()
{
    for (uint16 i = 0; i < NCPU; i++)
    {
        sem_init(&work_queue[i].count, 0, "work_count");
        fifo_init(&work_queue[i].queue);
        spin_init(&work_queue[i].lock, "work_lock");
        kthread_create(kthread_work_handler, NULL, "work_handler", i);
    }
}

void work_queue_push(void (*func)(void *), void *args)
{
    push_off();
    int cid = cpuid();
    struct work_struct *w = work_alloc(func, args);
    if (!w){
        panic("work push w is NULL\n");
    }

    spin_lock(&work_queue[cid].lock);
    // list_add_tail(&w->list,&work_queue[cid].queue.list);
    fifo_push(&work_queue[cid].queue, &w->list);
    
    sem_signal(&work_queue[cid].count);
    spin_unlock(&work_queue[cid].lock);
    // printk("wake hart: %d\n", cpuid());
    pop_off();
}
