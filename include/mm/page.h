#ifndef __PAGE_H__
#define __PAGE_H__

#include "std/stddef.h"
#include "mm/memlayout.h"
#include "lib/list.h"
#include "lib/atomic.h"
#include "lib/spinlock.h"

/*
 *
 * 注意，修改页面的标志需要持有 mem_map 的锁
 *
 */


// 页面标志位
#define PG_dirty (1 << 0)    // 页面脏
#define PG_locked (1 << 1)   // 锁定页面，不能进行页面交换或被其他进程修改（可用于写时复制）
#define PG_anon (1 << 2)     // 是否为匿名页面（非文件映射的页面），可用于堆栈、堆和内核
#define PG_reserved (1 << 3) // 是否为保留页面，避免被操作系统的分配器使用
#define PG_Slab (1 << 4)     // 用于 Slab

#define PG_FREE 0

// 描述每一个物理页面
struct page
{
    flags_t flags;
    atomic_t refcnt;
    struct list_head buddy;
    struct slab *slab;
    spinlock_t lock;
};

struct mem_map_struct
{
    // 所有物理页面的数组，下标为页框
    // 由于我们管理 0x8000_0000 以上 128M 的页面，因此实际使用中都要加上基地址页框的偏移了
    struct page pages[ALL_PFN];
    spinlock_t lock;
};

// 需要持有 mem_map 锁
extern struct mem_map_struct mem_map;
extern uint32 kernel_pfn_end;

extern int page_count(struct page *);
extern int page_is_free(struct page *);
extern void get_page(struct page *);
extern void put_page(struct page *);

#define I2PA(i) (((uint64)(i) << PGSHIFT) + KERNBASE)
#define PA2I(pa) (((uint64)(pa) - KERNBASE) >> PGSHIFT)

#define put_page_test(pg) \
    atomic_dec_and_test(&(pg)->refcnt)

#define put_page_test_1(pg) \
    atomic_dec_and_test_1(&(pg)->refcnt)

#define PA2PG(pa) \
    (struct page *)(mem_map.pages + PA2I(pa))

#define PG2PA(pg) \
    (I2PA((pg) - mem_map.pages))

#endif
