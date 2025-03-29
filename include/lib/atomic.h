#ifndef __ATOMIC_H__
#define __ATOMIC_H__
#include "std/stdio.h"
typedef struct
{
    volatile int counter;
} atomic_t;

#define ATOMIC_INIT(i) {(i)}

static inline void atomic_set(atomic_t *v, int i)
{
    __sync_lock_test_and_set(&v->counter, i);
}

static inline int atomic_read(const atomic_t *v)
{
    return __atomic_load_n(&v->counter, __ATOMIC_SEQ_CST);
}

static inline void atomic_add(int i, atomic_t *v)
{
    __asm__ __volatile__(
        "amoadd.w zero, %1, %0"
        : "+A"(v->counter)
        : "r"(i));
}

static inline void atomic_sub(int i, atomic_t *v)
{
    __asm__ __volatile__(
        "amoadd.w zero, %1, %0"
        : "+A"(v->counter)
        : "r"(-i));
}

static inline void atomic_inc(atomic_t *v)
{
    atomic_add(1, v);
}

static inline void atomic_dec(atomic_t *v)
{
    atomic_sub(1, v);
}

static inline int atomic_dec_and_test(atomic_t *v)
{
    int old;
    // 使用 amoadd.w 原子操作完成读取和自减
    __asm__ __volatile__(
        "amoadd.w %0, %2, (%1)"       // 自减操作，amoadd.w 返回旧值
        : "=r"(old)                   // 将旧值写入 old
        : "r"(&(v->counter)), "r"(-1) // 内存地址，-1 是加数
        : "memory"                    // 内存屏障，保证操作顺序
    );

    // 检查新值是否为零
    return old == 1; // amoadd.w 返回旧值，因此需要减 1 检查
}

static inline int atomic_dec_and_test_1(atomic_t *v)
{
    int old;
    // 使用 amoadd.w 原子操作完成读取和自减
    __asm__ __volatile__(
        "amoadd.w %0, %2, (%1)"       // 自减操作，amoadd.w 返回旧值
        : "=r"(old)                   // 将旧值写入 old
        : "r"(&(v->counter)), "r"(-1) // 内存地址，-1 是加数
        : "memory"                    // 内存屏障，保证操作顺序
    );

    // 检查原来的值是否大于1
    return (old > 1);
}
#endif
