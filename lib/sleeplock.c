#include "lib/sleeplock.h"
#include "std/stddef.h"
#include "core/proc.h"
#include "lib/mutex.h"

// 初始化睡眠锁，我们简单认为是信号量为1，有点类似于 Mutex
inline void sleep_init(sleeplock_t *lk, const char *name)
{
    mutex_init(lk, name);
}

inline int sleep_waiters_count(sleeplock_t *lk)
{
    return atomic_read(&lk->sem.value);
}

inline void sleep_init_zero(sleeplock_t *lk, const char *name)
{
    mutex_init_zero(lk, name);
}

// 申请睡眠锁
inline void sleep_on(sleeplock_t *lk)
{
    // 在这个信号量上睡眠，如果当前信号量忙
    mutex_lock(lk);
}

// 释放睡眠锁
inline void wake_up(sleeplock_t *lk)
{
    mutex_unlock(lk);
}

// 检测持有睡眠锁
inline int sleep_is_hold(sleeplock_t *lk)
{
    return mutex_is_hold(lk);
}