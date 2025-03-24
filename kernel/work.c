#include "core/work.h"

#include "core/proc.h"
#include "lib/list.h"
#include "lib/mutex.h"
#include "lib/semaphore.h"
#include "lib/spinlock.h"
#include "mm/kmalloc.h"

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
    spinlock_t lock;  // 保护队列

    semaphore_t count;
} work_queue[NCPU];

// debug_work_list
// debug_cpu_timer_list
// debug_cpu_shed_list

__attribute__((unused)) void debug_work_list()
{
    printk("debug_work_list:\n");
    for (int i = 0; i < NCPU; i++) {
        printk("cpu %d len %d\n", i, work_queue[i].queue.f_size);
    }
}

static struct work_struct *work_alloc(void (*func)(void *), void *args)
{
    struct work_struct *w = kmalloc(sizeof(struct work_struct), 0);
    if (!w) {
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
    push_off();
    int cid = cpuid();
    printk("hart %d wait\n", cpuid());
    pop_off();

    // ! sem 会切换CPU
    sem_wait(&work_queue[cid].count);
    printk("hart %d run\n", cid);

    spin_lock(&work_queue[cid].lock);
    struct list_head *node = fifo_pop(&work_queue[cid].queue);
    spin_unlock(&work_queue[cid].lock);
    if (!node) {
        return NULL;
    }
    struct work_struct *w = list_entry(node, struct work_struct, list);
    printk("worl: %p,%p\n",w->func,w->args);
    return w;
}

static void work_free(struct work_struct *w)
{
    kfree(w);
}

static __attribute__((noreturn)) void kthread_work_handler()
{
    for (;;) {
        struct work_struct *w = work_queue_pop();
        printk("work get 1 %d\n",myproc()->pid);
        if (w == NULL || (uint64)w->func < 0x1000 || (uint64)w->args < 0x1000)
            panic("SSSSSSSSSSS\n");
        printk("work get 2 %d\n",myproc()->pid);
        printk("worl 2: %p,%p\n",(uint64)w->func,(uint64)w->args);
        w->func(w->args);
        work_free(w);
    }
}

void work_queue_init()
{
    for (uint16 i = 0; i < NCPU; i++) {
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
    printk("wake hart: %d\n", cpuid());

    struct work_struct *w = work_alloc(func, args);
    if (!w) {
        panic("work push w is NULL\n");
    }

    spin_lock(&work_queue[cid].lock);
    fifo_push(&work_queue[cid].queue, &w->list);

    spin_unlock(&work_queue[cid].lock);

    sem_signal(&work_queue[cid].count);
    pop_off();
}
