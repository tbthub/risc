#ifndef __CONS_H__
#define __CONS_H__
#include "lib/spinlock.h"

#define INPUT_BUF_SIZE 128

// 直接声明结构体类型
struct cons_struct {
    spinlock_t lock;
    char buf[INPUT_BUF_SIZE];
    uint r; // Read index
    uint w; // Write index
    uint e; // Edit index
};

extern struct cons_struct cons;

void cons_init();
#endif