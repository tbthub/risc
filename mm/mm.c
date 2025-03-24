#include "mm/mm.h"

#include "defs.h"
#include "lib/list.h"
#include "lib/spinlock.h"
#include "lib/string.h"
#include "mm/buddy.h"
#include "mm/page.h"
#include "riscv.h"
#include "std/stddef.h"
#include "std/stdio.h"

extern void kmalloc_init();
extern void kmem_cache_init();
extern void first_all_page_init();

// 注：我们的伙伴算法，实际上只是管理 mem_map.pages 这个数组
// 并不涉及具体页面地址的管理
// 由于这个数组元素与页面地址一一对应的关系
// 我们仅在分配/释放页面的出入口有所处理

static struct
{
    spinlock_t lock;
    struct list_head free_lists[MAX_LEVEL];
} Buddy;

// 寻找当前 page 的伙伴
static struct page *find_buddy(const struct page *page, const int order)
{
    uint64 index = page - mem_map.pages;
    uint64 buddy_index = index ^ (1 << order);
    if (buddy_index >= ALL_PFN) {
        // 如果伙伴超出了内存页的范围，返回 NULL
        printk("OOM ERROR!\n");
        return NULL;
    }
    return mem_map.pages + buddy_index;
}

// 分配
static struct page *buddy_alloc(const int order)
{
    if (order < 0 || order > MAX_LEVEL_INDEX)
        return NULL;

    int i, j;
    struct page *page;
    struct page *buddy_page;

    spin_lock(&Buddy.lock);
    for (i = order; i < MAX_LEVEL; i++) {
        // 找到有一个不为空的
        if (!list_empty(&Buddy.free_lists[i])) {
            // 弹出一个 page
            page = list_entry(list_pop(&Buddy.free_lists[i]), struct page, buddy);

            // 拆分块直到满足所需 order
            for (j = i; j > order; j--) {
                // 寻找下一级的伙伴(这里的伙伴存在的话一定是空闲的)
                buddy_page = find_buddy(page, j - 1);
                if (!buddy_page)
                   goto bad;

                // 加到下一级的链表中
                list_add_head(&buddy_page->buddy, &Buddy.free_lists[j - 1]);
            }
            // 循环接受，该 page 也就是这个被拆分大块的起始地址（现在变成小块了）
            spin_unlock(&Buddy.lock);
            return page;
        }
    }
bad:
    // 没有找到
    spin_unlock(&Buddy.lock);
    panic("BUDDY!");
    return NULL;
}

// 回收
static void buddy_free(struct page *pg, const int order)
{
    int level;
    struct page *page = pg, *buddy_page;

    spin_lock(&Buddy.lock);

    // 向上合并伙伴
    for (level = order; level < MAX_LEVEL_INDEX; level++) {
        buddy_page = find_buddy(page, level);
        if (list_empty(&Buddy.free_lists[level]))
            break;
        // 伙伴不在，也就不用向上合并了
        // 如果页面不空闲，则已经被分配出去了，现在一定不在伙伴系统里面
        if (buddy_page == NULL || !page_is_free(buddy_page))
            break;

        // 如果有伙伴块的话
        list_del_init(&buddy_page->buddy);
        list_del_init(&page->buddy);

        // page 始终为位置更低的，这样最后的 page 就是最后大块的头儿
        page = page < buddy_page ? page : buddy_page;
    }
    list_add_head(&page->buddy, &Buddy.free_lists[level]);
    spin_unlock(&Buddy.lock);
}

// 初始化 Buddy 系统
static void buddy_init()
{
    uint64 i;

    spin_init(&Buddy.lock, "buddy");

    // 初始化伙伴的每个链表节点
    for (i = 0; i < MAX_LEVEL; i++)
        INIT_LIST_HEAD(&Buddy.free_lists[i]);

    for (i = kernel_pfn_end + 1; i < ALL_PFN; i += MAX_LEVEL_COUNT)
        list_add_head(&mem_map.pages[i].buddy, &Buddy.free_lists[MAX_LEVEL_INDEX]);

    // 处理最后一点不足 max_level_count 的，也就是说，大概率是最后一个其实没有满
    // 我们把这个假的弹出来
    list_pop(&Buddy.free_lists[MAX_LEVEL_INDEX]);

    uint end_free_count = (ALL_PFN - kernel_pfn_end) % MAX_LEVEL_COUNT;

    // 假装分配
    for (i = ALL_PFN - end_free_count; i < ALL_PFN; i++)
        get_page(&mem_map.pages[i]);

    __sync_synchronize();

    // 一个一个回收
    for (i = ALL_PFN - end_free_count; i < ALL_PFN; i++)
        free_page(mem_map.pages + i);

    // printk("Buddy system: %d blocks\n", ALL_PFN - kernel_pfn_end);
}

// 分配 pages
struct page *alloc_pages(uint32 flags, const int order)
{
    struct page *pages = buddy_alloc(order);
    get_page(pages);
    return pages;
}

void *__alloc_pages(uint32 flags, const int order)
{
    return (void *)PG2PA(alloc_pages(flags, order));
}

// 分配一个 page
struct page *alloc_page(uint32 flags)
{
    return alloc_pages(flags, 0);
}

void *__alloc_page(uint32 flags)
{
    void *addr = (void *)PG2PA(alloc_pages(flags, 0));
    memset(addr, 0, PGSIZE);
    return addr;
}

// 释放 pages
void free_pages(struct page *pages, const int order)
{
    assert(page_count(pages) > 0, "free_pages");
    if (put_page_test(pages))
        buddy_free(pages, order);
}

inline void __free_pages(void *addr, const int order)
{
    free_pages(PA2PG(addr), order);
}

// 释放一个 page
inline void free_page(struct page *page)
{
    free_pages(page, 0);
}

inline void __free_page(void *addr)
{
    free_pages(PA2PG(addr), 0);
}

void mm_debug()
{
    for (int i = 0; i < MAX_LEVEL; i++)
    {
        uint len = list_len(&Buddy.free_lists[i]);
        printk("{ L: %d, N: %d }\t", i, len);
    }
    printk("\n");
}

// 内存管理初始化: page、buddy、kmem_cache、kmalloc
void mm_init()
{
    first_all_page_init();
    buddy_init();
    kmem_cache_init();
    kmalloc_init();
    printk("mm_init ok\n");
    mm_debug();
}


