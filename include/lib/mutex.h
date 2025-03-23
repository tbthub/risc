#ifndef __MUTEX_H__
#define __MUTEX_H__
#include "lib/semaphore.h"

// 基于信号量机制的互斥量
// 由于会睡眠，不能在临界区或者中断上下文中使用

typedef struct mutex
{
    int locked;                 // Is the lock held?
    semaphore_t sem;            // for sleeping
    struct thread_info *thread; // Process holding lock
} mutex_t;

#define MUTEX_LOCK 1
#define MUTEX_UNLOCK 0

void mutex_init(mutex_t *mutex, const char *name);
void mutex_init_zero(mutex_t *mutex, const char *name);
void mutex_lock(mutex_t *mutex);
void mutex_unlock(mutex_t *mutex);
int mutex_is_hold(mutex_t *mutex);

#endif