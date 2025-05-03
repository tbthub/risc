#include "mm/page.h"
#include "mm/memlayout.h"
#include "lib/atomic.h"
#include "defs.h"
#include "lib/list.h"
#include "riscv.h"
#include "std/stdio.h"
#include "core/locks/spinlock.h"
#include "defs.h"
#include "mm/slab.h"

// 需要持有 mem_map 锁
struct mem_map_struct mem_map;
// 注：mem_map的pages位于 bss段，编译后的大数组并不占用
// 可执行文件空间，但是加载的时候会变大，
// 截止25/4/25，这个数组占用 93.6% 的BSS大小

// first address after kernel. defined by kernel.ld.
// 由编译器最后计算出来，位于代码段和数据段的顶端
uint32 kernel_pfn_end;

static inline void init_page(struct page *pg, flags_t flags)
{
	pg->flags = flags;
	atomic_set(&pg->refcnt, PG_FREE);
	INIT_LIST_HEAD(&pg->buddy);
	pg->slab = NULL;
	pg->order = MAX_LEVEL;
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


void set_page_kmalloc_page(struct page *page)
{
	SET_FLAG(&page->flags,PG_KMALLOC_PAGE);
}

uint32 test_page_kmalloc_page(struct page *page)
{
	return TEST_FLAG(&page->flags,PG_KMALLOC_PAGE);
}

void clear_page_kmalloc_page(struct page *page)
{
	CLEAR_FLAG(&page->flags,PG_KMALLOC_PAGE);
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

	// 不归伙伴系统管
	for (i = 0; i < kernel_pfn_end; i++) {
		get_page(&mem_map.pages[i]);
		SET_FLAG(&mem_map.pages[i].flags, PG_reserved | PG_anon);
	}
}
