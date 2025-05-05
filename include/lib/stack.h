#ifndef __STACK_H__
#define __STACK_H__
#include "std/stddef.h"


// base: 起始地址（低）
// top:  栈顶指针（高）
#define STACK_UNLIMIT -1
typedef struct
{
    uint64_t *top;
    uint64_t *base;
    int32_t max_size;
}sstack_t;

static inline int sstack_is_empty(sstack_t *ss) {
    return ss->top == ss->base;
}

static inline int sstack_is_full(sstack_t *ss) {
    if (ss->max_size == STACK_UNLIMIT) {
        return 0;
    }
    return (ss->top - ss->base) == ss->max_size;
}

static inline void sstack_init(sstack_t *ss, uint64_t *base, int32_t max_size)
{
    ss->top = ss->base = base;
    ss->max_size = max_size;
}

static inline int sstack_push(sstack_t *ss, uint64_t addr)
{
    if (sstack_is_full(ss)) {
        return -1;
    }
    *ss->top = addr;
    ss->top++;
    return 0;
}

static inline uint64_t sstack_pop(sstack_t *ss)
{
    if(sstack_is_empty(ss))
        return 0;

    ss->top--;
    return *ss->top;
}

#endif
