#ifndef __BUF_H__
#define __BUF_H__

#include "lib/list.h"
#include "lib/hash.h"
#include "lib/atomic.h"
#include "core/locks/spinlock.h"
#include "core/locks/sleeplock.h"
#include "core/locks/semaphore.h"

#include "std/stddef.h"
#include "core/timer.h"

struct gendisk;

#define BH_Dirty (1 << 0) // 脏，需要写回磁盘
#define BH_New (1 << 1)   // 新的，所指向的页面是新的
#define BH_Fixed (1 << 3) // 常驻内存
#define BH_Valid (1 << 4)   // 有效

#define BH_Visited (1 << 31) // 用于 LRU2 检测是否再次访问
#define BH_Active (1 << 30)  // 用于 LRU2 检测是否再次访问

struct buf_head
{
    struct gendisk *gd; // 哪一个通用块

    flags_t flags;
    uint32 blockno; // 起始块号

    sleeplock_t lock; // 用于 IO 互斥
    atomic_t refcnt;

    hash_node_t bh_node;    // 哈希节点
    struct list_head lru;   // lru
    struct list_head dirty; // dirty_list

    void *page;
};

struct bhash_struct
{
    struct gendisk *gd;

    // 缓存哈希
    spinlock_t lock;
    struct hash_table buf_hash_table;

    // 这里采用 LRU2 管理
    struct list_head active_list;   // 活跃的LRU链
    struct list_head inactive_list; // 不活跃
    uint32 active_count;
    uint32 inactive_count;

    struct list_head dirty_list; // 脏链
};

extern void bhash_init(struct bhash_struct *bhash, struct gendisk *gd);

extern struct buf_head *buf_get(struct gendisk *gd, uint blockno);

// 注意：下面三个函数会陷入睡眠，不允许持有 bhash 锁
extern void buf_release(struct buf_head *b, int is_dirty);
extern void buf_pin(struct buf_head *b);
extern void buf_unpin(struct buf_head *b);

#define buf_used(buf) \
    CLEAR_FLAG(&(buf)->flags, BH_New)

#define buf_is_new(buf) \
    TEST_FLAG(&(buf)->flags, BH_New)

#endif
