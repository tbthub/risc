#ifndef __TIMER_H__
#define __TIMER_H__
#include "std/stddef.h"
#include "lib/list.h"

struct thread_info;
typedef uint64_t ticks_t;


typedef struct timer
{
    void (*callback)(void *); // 回调函数
    void *args;
    uint64_t init_time;
    uint64_t during_time; // 经过时间
    struct list_head list;
    int count; // 调用次数
    int block; // 是否会堵塞
} timer_t;

#define NO_RESTRICT -1

#define TIMER_NO_BLOCK 0
#define TIMER_BLOCK 1

extern void time_init();
extern void time_update();
extern ticks_t get_cur_time();
extern void thread_timer_sleep(struct thread_info *thread, uint64_t down_time);
extern timer_t *timer_create(void (*callback)(void *), void *args, uint64_t during_time, int count, int is_block);
#endif
