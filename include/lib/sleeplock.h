#ifndef __SLEEPLOCK_H__
#define __SLEEPLOCK_H__
#include "lib/spinlock.h"
#include "lib/semaphore.h"
#include "lib/mutex.h"

// 我们直接用互斥量来代替睡眠锁
// 注意，睡眠锁只能在进程上下文使用！！！
typedef mutex_t sleeplock_t;

extern void sleep_init(sleeplock_t *lk, const char *name);
extern void sleep_init_zero(sleeplock_t *lk, const char *name);
extern void sleep_on(sleeplock_t *lk);
extern void wake_up(sleeplock_t *lk);
extern int sleep_is_hold(sleeplock_t *lk);
extern int sleep_waiters_count(sleeplock_t *lk);

#endif