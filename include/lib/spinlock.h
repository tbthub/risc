// 自旋锁
#ifndef __SPINLOCK_H
#define __SPINLOCK_H

#include "std/stddef.h"


typedef struct {
    int lock;
    char name[12];
    int cpuid;
}spinlock_t;

#define SPIN_UNLOCKED 0
#define SPIN_LOCKED 1


void spin_init(spinlock_t *lock,const char *);
void spin_lock(spinlock_t *lock);
void spin_unlock(spinlock_t *lock);
void pop_off();
void push_off();
int holding(spinlock_t *lock);
#endif
