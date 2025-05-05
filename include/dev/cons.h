#ifndef __CONS_H__
#define __CONS_H__
#include "core/locks/spinlock.h"

#define INPUT_BUF_SIZE 128

// 直接声明结构体类型
struct cons_struct {
    spinlock_t lock;
    char buf[INPUT_BUF_SIZE];
    uint32_t r; // Read index
    uint32_t w; // Write index
    uint32_t e; // Edit index
};

extern struct cons_struct cons;

void cons_init();
#endif
