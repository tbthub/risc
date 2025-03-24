// #include "core/proc.h"
// #include "core/work.h"
// #include "lib/list.h"
// #include "lib/mutex.h"
// #include "lib/semaphore.h"
// #include "lib/spinlock.h"
// #include "mm/kmalloc.h"

// // 工作队列机制

// struct work_struct
// {
//     void (*func)(void *);
//     void *args;
//     struct list_head list;
// };

// static struct work_queue_struct
// {
//     struct fifo queue;
//     spinlock_t lock;  // 保护队列
//     struct thread_info *thread;
//     semaphore_t count;
// } work_queue[NCPU];

// static struct thread_info *workers[NCPU];

// __attribute__((unused)) void debug_work_list()
// {
//     printk("debug_work_list:\n");
//     for (int i = 0; i < NCPU; i++) {
//         printk("cpu %d len %d\n", i, work_queue[i].queue.f_size);
//     }
// }

// static struct work_struct *work_alloc(void (*func)(void *), void *args)
// {
//     struct work_struct *w = kmalloc(sizeof(struct work_struct), 0);
//     if (!w) {
//         printk("work.c [work_alloc]: work_struct memory allocation failed\n");
//         return NULL;
//     }
//     w->func = func;
//     w->args = args;
//     INIT_LIST_HEAD(&w->list);
//     return w;
// }

// static __attribute__((noreturn)) void kthread_work_handler()
// {
//     for (;;) {
//         struct work_struct *w = work_queue_pop();
//         w->func(w->args);
//         work_free(w);
//     }
// }

// void work_queue_init()
// {
//     for (uint16 i = 0; i < NCPU; i++) {
//         fifo_init(&work_queue[i].queue);
//         spin_init(&work_queue[i].lock, "work_lock");
//         kthread_create(kthread_work_handler, NULL, "work_handler", i);
//     }
// }

// void work_queue_push(void (*func)(void *), void *args)
// {
//     push_off();
//     int cid = cpuid();
//     pop_off();

//     struct work_struct *w = work_alloc(func, args);
//     if (!w) {
//         panic("work push w is NULL\n");
//     }

//     spin_lock(&work_queue[cid].lock);
//     fifo_push(&work_queue[cid].queue, &w->list);
//     spin_unlock(&work_queue[cid].lock);

//     sem_signal(&work_queue[cid].count);
// }
