#include "core/locks/rwlock.h"
#include "core/proc.h"
#include "core/sched.h"
#include "core/timer.h"
#include "std/stdio.h"

static rwlock_t rw;
static int ARG = 10;

// 线程函数：读者
static void reader(void *arg)
{
    int *id = (int *)arg;
    read_lock(&rw);

    printk("reader %d raed arg: %d\n", myproc()->pid, *id);
    thread_timer_sleep(myproc(), 100);  // 模拟读操作耗时

    read_unlock(&rw);
}

// 线程函数：写者
static void writer(void *arg)
{
    int *id = (int *)arg;
    write_lock(&rw);

    printk("writer %d write arg %d -> %d\n", myproc()->pid, *id, myproc()->pid);
    *id = myproc()->pid;
    thread_timer_sleep(myproc(), 100);  // 模拟写操作耗时

    write_unlock(&rw);
}

// 运行测试函数
static void run_test(int priority)
{
    set_rw_prior(&rw, priority);
    kthread_create(myproc(), writer, &ARG, "writer", NO_CPU_AFF);
    kthread_create(myproc(), writer, &ARG, "writer", NO_CPU_AFF);
    kthread_create(myproc(), reader, &ARG, "reader", NO_CPU_AFF);
    kthread_create(myproc(), reader, &ARG, "reader", NO_CPU_AFF);
    kthread_create(myproc(), reader, &ARG, "reader", NO_CPU_AFF);
    kthread_create(myproc(), reader, &ARG, "reader", NO_CPU_AFF);
    kthread_create(myproc(), writer, &ARG, "writer", NO_CPU_AFF);
    kthread_create(myproc(), reader, &ARG, "reader", NO_CPU_AFF);
    kthread_create(myproc(), writer, &ARG, "writer", NO_CPU_AFF);
    kthread_create(myproc(), writer, &ARG, "writer", NO_CPU_AFF);
    kthread_create(myproc(), writer, &ARG, "writer", NO_CPU_AFF);
    kthread_create(myproc(), writer, &ARG, "writer", NO_CPU_AFF);

    thread_timer_sleep(myproc(), 1000);
    kthread_create(myproc(), reader, &ARG, "reader", NO_CPU_AFF);
    kthread_create(myproc(), reader, &ARG, "reader", NO_CPU_AFF);
    kthread_create(myproc(), reader, &ARG, "reader", NO_CPU_AFF);
    kthread_create(myproc(), writer, &ARG, "writer", NO_CPU_AFF);
    kthread_create(myproc(), reader, &ARG, "reader", NO_CPU_AFF);
    kthread_create(myproc(), reader, &ARG, "reader", NO_CPU_AFF);
    kthread_create(myproc(), reader, &ARG, "reader", NO_CPU_AFF);
    kthread_create(myproc(), reader, &ARG, "reader", NO_CPU_AFF);
    kthread_create(myproc(), writer, &ARG, "writer", NO_CPU_AFF);
    kthread_create(myproc(), reader, &ARG, "reader", NO_CPU_AFF);
}

static void __rw_test()
{
    // 初始化读写锁
    rwlock_init(&rw, 0, "TestRWLock");

    // 测试读者优先模式
    run_test(0);

    // 测试写者优先模式
    // run_test(1);
}

void rw_test()
{
    kthread_create(myproc(), __rw_test, NULL, "rw_test", NO_CPU_AFF);
}
