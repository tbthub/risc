#include "lib/semaphore.h"
#include "std/stdio.h"
#include "core/proc.h"
static semaphore_t sem1;
static semaphore_t sem2;
static semaphore_t sem3;
static semaphore_t sem4;

static void thread1()
{
    while (1)
    {
        sem_wait(&sem1);
        printk("A, cpu:%d:  wake up B intr: %d\n", cpuid(), intr_get());
        sem_signal(&sem2);
    }
}

static void thread2()
{
    while (1)
    {
        sem_wait(&sem2);
        printk("B, cpu:%d:  wake up C intr: %d\n", cpuid(), intr_get());
        sem_signal(&sem3);
    }
}

static void thread3()
{
    while (1)
    {
        sem_wait(&sem3);
        printk("C, cpu:%d:  wake up D intr: %d\n", cpuid(), intr_get());
        sem_signal(&sem4);
    }
}

static void thread4()
{
    while (1)
    {
        sem_wait(&sem4);
        printk("D, cpu:%d:  wake up A intr: %d\n", cpuid(), intr_get());
        sem_signal(&sem1);
    }
}

// 我们预想的顺序是 ACBD..ACBD..
void sem_test()
{
    if (cpuid() == 0)
    {
        sem_init(&sem1, 0, "test1");
        sem_init(&sem2, 0, "test2");
        sem_init(&sem3, 0, "test3");
        sem_init(&sem4, 0, "test4");
        kthread_create(thread1, NULL, "thread_A",NO_CPU_AFF);
        kthread_create(thread2, NULL, "thread_B",NO_CPU_AFF);
        kthread_create(thread3, NULL, "thread_C",NO_CPU_AFF);
        kthread_create(thread4, NULL, "thread_D",NO_CPU_AFF);
    }
}