#ifndef __DEVS_H__
#define __DEVS_H__
#include "std/stddef.h"
#include "lib/spinlock.h"
#include "lib/list.h"
#include "dev/blk/blk_dev.h"

// 全局设备链。
// 1. 管理设备号的分配
// 2. 注册设备加入

extern struct dev_lists_struct dev_lists;

extern dev_t devno_alloc();
extern void devs_init();
extern void devs_add(struct list_head * _new);

#define DEV_READ 0
#define DEV_WRITE 1

#endif