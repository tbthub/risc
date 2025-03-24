#include "lib/semaphore.h"
#include "lib/fifo.h"
#include "lib/spinlock.h"
#include "core/proc.h"
#include "core/sched.h"
#include "lib/atomic.h"
#include "riscv.h"

void sem_init(semaphore_t *sem, int value, const char *name)
{
    atomic_set(&sem->value, value);
    spin_init(&sem->lock, name);
    fifo_init(&sem->waiters);
}

void sem_wait(semaphore_t *sem)
{
    if(!sem){
        panic("sem wait is NULL\n");
    }
    int intr = intr_get();
    spin_lock(&sem->lock);
    atomic_dec(&sem->value);
    if (atomic_read(&sem->value) < 0)
    {
        struct thread_info *thread = myproc();
        // 在加入等待队列之前，释放信号量锁，防止死锁
        spin_lock(&thread->lock);
        thread->state = SLEEPING;
        fifo_push(&sem->waiters, &thread->sched);

        spin_unlock(&sem->lock);
        sched(intr);
	printk("sem wait up,%d\n",thread->pid);
    }
    else
    {
        // 如果信号量大于 0，则直接释放锁
        spin_unlock(&sem->lock);
    }
}

void sem_signal(semaphore_t *sem)
{
    spin_lock(&sem->lock);
    atomic_inc(&sem->value);
    // 如果信号量小于等于 0，说明有线程在等待
    if (atomic_read(&sem->value) <= 0)
    {
        
        struct list_head *waiter = fifo_pop(&sem->waiters);
        if (waiter != NULL)
        {
            struct thread_info *thread = list_entry(waiter, struct thread_info, sched);
            wakeup_process(thread);
            // printk("sem pop queue, name: %s\n",thread->name);
        }
        else
        {
            panic("sem_signal: sem \"%s\" queue may have a race condition. "
                  "all size: %d, but queue-len: %d\n",
                  sem->lock.name, sem->waiters.f_size,
                  list_len(&sem->waiters.f_list));
        }
    }
    spin_unlock(&sem->lock);
}
