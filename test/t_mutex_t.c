#include "lib/mutex.h"
#include "std/stdio.h"
#include "core/proc.h"
#include "core/sched.h"

static mutex_t mutex1;
static mutex_t mutex2;
static mutex_t mutex3;
static mutex_t mutex4;

static void thread1()
{
    while (1)
    {
        mutex_lock(&mutex1);
        printk("A cpu:%d \n", cpuid());
        mutex_unlock(&mutex2);
        mutex_unlock(&mutex3);
    }
}

static void thread2()
{
    while (1)
    {
        mutex_lock(&mutex2);
        printk("B cpu:%d \n", cpuid());
        mutex_unlock(&mutex4);
    }
}

static void thread3()
{
    while (1)
    {
        mutex_lock(&mutex3);
        printk("C cpu:%d \n", cpuid());
        mutex_unlock(&mutex4);
    }
}

static void thread4()
{
    while (1)
    {
        mutex_lock(&mutex4);
        printk("D cpu:%d \n", cpuid());
        mutex_unlock(&mutex1);
    }
}

void mutex_test()
{
    // 初极狭，才通人；复行数十步，豁然开朗。
    if (cpuid() == 0)
    {
        mutex_init(&mutex1, "test1");
        mutex_init_zero(&mutex2, "test2");
        mutex_init_zero(&mutex3, "test3");
        mutex_init_zero(&mutex4, "test4");

        kthread_create(thread1,NULL, "thread_A",NO_CPU_AFF);
        kthread_create(thread2,NULL, "thread_B",NO_CPU_AFF);
        kthread_create(thread3,NULL, "thread_C",NO_CPU_AFF);
        kthread_create(thread4,NULL, "thread_D",NO_CPU_AFF);
    }
}