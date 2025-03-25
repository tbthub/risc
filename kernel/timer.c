#include "core/timer.h"

#include "core/proc.h"
#include "core/work.h"
#include "lib/fifo.h"
#include "lib/sleeplock.h"
#include "lib/spinlock.h"
#include "mm/kmalloc.h"
#include "mm/slab.h"
#include "std/stdio.h"

static ticks_t sys_ticks;

// 我们对每个CPU都分配一个定时器,避免多 CPU 造成的竞争，
// 同时，由于时钟中断后检查到期，不会产生调度，因此，也不存在加入|删除队列的竞争
struct cpu_timer_struct
{
    spinlock_t lock;
    struct list_head list;
} cpu_timer[NCPU];

__attribute__((unused)) void debug_cpu_timer_list()
{
    printk("\ndebug_cpu_timer_list:\n");
    for (int i = 0; i < NCPU; i++) {
        printk("cpu %d len %d\n", i, list_len(&cpu_timer[i].list));
    }
}

inline void time_init()
{
    sys_ticks = 0;
    for (uint32 i = 0; i < NCPU; i++) {
        spin_init(&cpu_timer[i].lock, "cpu_timer");
        INIT_LIST_HEAD(&cpu_timer[i].list);
    }
}

// 我们以后改为线程池，暂时先用创建线程代替
// 且目前我们还没有写进程资源回收，也就是会越来越多
static void timer_wake(timer_t *t)
{
    if (t->block == TIMER_NO_BLOCK)  // 不堵塞的话，直接使用工作队列就行
    {
        work_queue_push(t->callback, t->args);
    }
    else if (t->block == TIMER_BLOCK)  // 会堵塞，则创建内核线程
    {
        kthread_create(get_init(),t->callback, t->args, "ktimer", NO_CPU_AFF);
    }
    else
        printk("timer.c [timer_wake]: TIMER_NO_BLOCK?TIMER_BLOCK");
}

// 销毁内核定时器
static void timer_del(timer_t *t)
{
    kmem_cache_free(&timer_kmem_cache, t);
}

inline ticks_t get_cur_time()
{
    return sys_ticks;
}

// 每次时钟中断，都要检查是否到期
static void timer_try_wake()
{
    // printk("timer_try_wake\n");
    timer_t *t;
    timer_t *tmp;
    struct cpu_timer_struct *ct = &cpu_timer[cpuid()];
    spin_lock(&ct->lock);
    list_for_each_entry_safe(t, tmp, &ct->list, list)
    {
        if(t == NULL)
            panic("t == NULL\n");
        // printk("%d,%d\n", sys_ticks, t->init_time + t->during_time);
        if (sys_ticks >= t->init_time + t->during_time)  // 到期了
        {
            t->init_time = sys_ticks;
            if (t->count != NO_RESTRICT) {
                t->count--;
                if (t->count <= 0) {
                    list_del_init(&t->list);
                    timer_wake(t);
                    timer_del(t);
                    continue;
                }
            }
            timer_wake(t);
        }
    }
    spin_unlock(&ct->lock);
}

// 现在是在中断的情况下，是绝对不允许睡眠的
inline void time_update()
{
    // 这个就算改不及时也没事，也就是说 CPU0改了后，
    // 哪怕CPU1看到的是以前的，也就只差一个时钟中断而已，很快就来了。没必要使用自旋锁

    if (cpuid() == 0)
        sys_ticks++;
    
    // 每个cpu每3次唤醒一次，这样来说实际上是有点不能及时唤醒的，
    // 应该会差
    // 1,2或者更多个（如果此时cpu还在关中断）时钟周期，但是这样可以大大减少唤醒次数
    // 这里没有必要对 sys_ticks 强加锁
    if (sys_ticks % 3 == 0) {
        timer_try_wake();
    }
}

// 需要在关中断情况下执行
static void assign_cpu(timer_t *t)
{
    struct cpu_timer_struct *ct = &cpu_timer[cpuid()];
    spin_lock(&ct->lock);
    list_add_head(&t->list, &ct->list);
    spin_unlock(&ct->lock);
}

// 创建的内核定时器 需要在关中断情况下执行
static timer_t *timer_create(void (*callback)(void *), void *args, uint64 during_time, int count, int is_block)
{
    timer_t *t = (timer_t *)kmem_cache_alloc(&timer_kmem_cache);
    if (!t) {
        printk("timer.c [timer_create]: Timer memory request failed\n");
        return NULL;
    }
    t->callback = callback;
    t->args = args;
    t->init_time = get_cur_time();
    t->during_time = during_time;
    t->count = count;
    t->block = is_block;
    assign_cpu(t);
    return t;
}

static void timer_waker_up(void *args)
{
    struct thread_info *t = (struct thread_info *)args;
    wakeup_process(t);
}

void thread_timer_sleep(struct thread_info *thread, uint64 down_time)
{
    spin_lock(&thread->lock);
    thread->state = SLEEPING;
    timer_create(timer_waker_up,thread,down_time, 1, TIMER_NO_BLOCK);
    sched();
    // 被唤醒后返回，从这里继续
}

int do_sleep(uint64 ticks)
{
    struct thread_info *t = myproc();
    assert(t != NULL, "do_sleep\n");
    thread_timer_sleep(t, ticks);
    return 0;
}
