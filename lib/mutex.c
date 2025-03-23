#include "lib/semaphore.h"
#include "lib/mutex.h"
#include "std/stddef.h"
#include "core/proc.h"

void mutex_init(mutex_t *mutex, const char *name)
{
    sem_init(&mutex->sem, MUTEX_LOCK, name);
    mutex->locked = MUTEX_LOCK;
    mutex->thread = NULL;
}

void mutex_init_zero(mutex_t *mutex, const char *name)
{
    sem_init(&mutex->sem, MUTEX_UNLOCK, name);
    mutex->locked = MUTEX_UNLOCK;
    mutex->thread = NULL;
}

void mutex_lock(mutex_t *mutex)
{
    sem_wait(&mutex->sem);

    // 被唤醒后继续执行
    mutex->locked = MUTEX_LOCK;
    mutex->thread = myproc();
}

void mutex_unlock(mutex_t *mutex)
{
    mutex->locked = MUTEX_UNLOCK;
    mutex->thread = NULL;

    sem_signal(&mutex->sem);
}

int mutex_is_hold(mutex_t *mutex)
{
    int r;
    spin_lock(&mutex->sem.lock);
    r = mutex->locked && (mutex->thread == myproc());
    spin_unlock(&mutex->sem.lock);
    return r;
}