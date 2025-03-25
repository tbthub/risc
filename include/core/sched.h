#ifndef __SCHED_H__
#define __SCHED_H__

#include "lib/spinlock.h"
#include "lib/list.h"

struct thread_info;

struct sched_struct
{
    spinlock_t lock;
    struct list_head run;
    struct list_head out;
};

#define NO_CPU_AFF -1

extern void sched();
extern void yield();
extern void scheduler();

extern void sched_init();
extern void wakeup_process(struct thread_info *thread);
extern struct thread_info * kthread_create(struct thread_info *pa, void (*func)(void *), void *args, const char *name, int cpu_affinity);
extern void debug_cpu_shed_list() __attribute__((unused));

#endif
