#include "mm/page.h"
#include "mm/memlayout.h"
#include "lib/atomic.h"
#include "defs.h"
#include "lib/list.h"
#include "riscv.h"
#include "std/stdio.h"
#include "lib/spinlock.h"
#include "defs.h"
#include "mm/slab.h"

// 需要持有 mem_map 锁
struct mem_map_struct mem_map;

// first address after kernel. defined by kernel.ld.
// 由编译器最后计算出来，位于代码段和数据段的顶端
uint32 kernel_pfn_end;

static inline void init_page(struct page *pg, flags_t flags)
{
	pg->flags = flags;
	atomic_set(&pg->refcnt, PG_FREE);
	INIT_LIST_HEAD(&pg->buddy);
	pg->slab = NULL;
	spin_init(&pg->lock, "page");
}

inline int page_count(struct page *pg)
{
	return atomic_read(&pg->refcnt);
}

// 检查一个块是否为空闲的伙伴块
inline int page_is_free(struct page *page)
{
	return page_count(page) == PG_FREE;
}

// 引用 +1
inline void get_page(struct page *page)
{
	atomic_inc(&page->refcnt);
}

// 引用 -1
inline void put_page(struct page *page)
{
	atomic_dec(&page->refcnt);
}

inline flags_t *page_flags(struct page *page)
{
	return &page->flags;
}

struct slab *page_slab(struct page *page)
{
	return page->slab;
}

void set_page_slab(struct page *page, struct slab *slab)
{
	SET_FLAG(&page->flags, PG_Slab);
	page->slab = slab;
}

void clear_page_slab(struct page *page)
{
	CLEAR_FLAG(&page->flags, PG_Slab);
	page->slab = NULL;
}

// 首次初始化
void first_all_page_init()
{
	int i;
	// 内核代码数据空间
	kernel_pfn_end = END_PFN - KERNBASE_PFN;
	spin_init(&mem_map.lock, "mem_map");
	// 省去条件判断写成两个循环，额，会快一点？
	for (i = 0; i < ALL_PFN; i++)
		init_page(&mem_map.pages[i], 0);

	for (i = 0; i < kernel_pfn_end; i++) {
		get_page(&mem_map.pages[i]);
		SET_FLAG(&mem_map.pages[i].flags, PG_reserved | PG_anon);
	}
}
