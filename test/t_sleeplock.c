#include "lib/sleeplock.h"
#include "std/stdio.h"
#include "core/proc.h"
static sleeplock_t lock1;
static sleeplock_t lock2;
static sleeplock_t lock3;
static sleeplock_t lock4;

static void thread1()
{
    while (1)
    {
        sleep_on(&lock1);
        printk("A, cpu:%d:  wake up B intr: %d\n", cpuid(), intr_get());
        wake_up(&lock2);
    }
}

static void thread2()
{
    while (1)
    {
        sleep_on(&lock2);
        printk("B, cpu:%d:  wake up C intr: %d\n", cpuid(), intr_get());
        wake_up(&lock3);
    }
}

static void thread3()
{
    while (1)
    {
        sleep_on(&lock3);
        printk("C, cpu:%d:  wake up D intr: %d\n", cpuid(), intr_get());
        wake_up(&lock4);
    }
}

static void thread4()
{
    while (1)
    {
        sleep_on(&lock4);
        printk("D, cpu:%d:  wake up A intr: %d\n", cpuid(), intr_get());
        wake_up(&lock1);
    }
}

// 我们预想的顺序是 ACBD..ACBD..
void sleep_test()
{
    if (cpuid() == 0)
    {
        // 由于睡眠锁初始化应该默认是1，没有为0的，我们直接使用 mutex_init_zero 进行初始化
        sleep_init(&lock1, "test1");
        mutex_init_zero(&lock2, "test2");
        mutex_init_zero(&lock3, "test3");
        mutex_init_zero(&lock4, "test4");

        kthread_create(thread1, NULL,"thread_A",NO_CPU_AFF);
        kthread_create(thread2, NULL,"thread_B",NO_CPU_AFF);
        kthread_create(thread3, NULL,"thread_C",NO_CPU_AFF);
        kthread_create(thread4, NULL,"thread_D",NO_CPU_AFF);
    }
}