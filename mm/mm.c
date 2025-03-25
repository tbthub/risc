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
    uint64 buddy_index = index ^ (1UL << order);
    // printk("ord: %d, i:%d, i1: %d\n",order, index, buddy_index);
    if (buddy_index >= ALL_PFN || buddy_index < 0) {
        // 如果伙伴超出了内存页的范围，返回 NULL
        printk("OOM ERROR!\n");
        return NULL;
    }
    return mem_map.pages + buddy_index;
}

void mm_debug()
{
    for (int i = 0; i < MAX_LEVEL; i++) {
        uint len = list_len(&Buddy.free_lists[i]);
        printk("{ L: %d, N: %d }\t", i, len);
    }
    printk("\n");
}

// 分配
static struct page *buddy_alloc(const int order)
{
    // printk("\n----\n");
    // printk("order: %d\n", order);
    // mm_debug();
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
            // printk("page: %p l: %d i: %d\n", PG2PA(page), i, page - mem_map.pages);
            // if(PG2PA(page) == 0x0000000087b4b000){
            //     // 申请3个
            //     printk("1");
            // }

            // if(PG2PA(page) == 0x0000000087b51000){
            //     // 单独的
            //     printk("1");
            // }

            // 拆分块直到满足所需 order
            for (j = i; j > order; j--) {
                // 寻找下一级的伙伴(这里的伙伴存在的话一定是空闲的)
                buddy_page = find_buddy(page, j - 1);
                // printk("bu: %p, l: %d i: %d\n", PG2PA(buddy_page), j - 1, buddy_page - mem_map.pages);
                if (!buddy_page)
                    goto bad;

                // if(!page_is_free(buddy_page))
                //     break;

                // 加到下一级的链表中
                list_add_head(&buddy_page->buddy, &Buddy.free_lists[j - 1]);
            }
            // mm_debug();

            if (page_count(page) > 0)
                panic("page_count\n");

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

        // 伙伴不在，也就不用向上合并了
        // 如果页面不空闲，则已经被分配出去了，现在一定不在伙伴系统里面
        if (buddy_page == NULL || !page_is_free(buddy_page))
            break;

        // 如果有伙伴块的话
        list_del(&buddy_page->buddy);
        list_del(&page->buddy);

        // page 始终为位置更低的，这样最后的 page 就是最后大块的头儿
        page = min(page, buddy_page);
    }
    list_add_head(&page->buddy, &Buddy.free_lists[level]);
    spin_unlock(&Buddy.lock);
}

// 初始化 Buddy 系统
static void buddy_init()
{
    uint64 i;

    spin_init(&Buddy.lock, "buddy");

    // 初始化所有链表
    for (i = 0; i < MAX_LEVEL; i++)
        INIT_LIST_HEAD(&Buddy.free_lists[i]);

    // 计算最高阶块的大小
    uint64 max_order = MAX_LEVEL_INDEX;
    uint64 chunk_size = 1UL << max_order;  // 块大小为 2^max_order 页

    // 计算对齐后的起始地址（向上取整到 chunk_size 的倍数）
    uint64 start_pfn = (kernel_pfn_end + chunk_size) / chunk_size * chunk_size;
    
    // 初始化最高阶块
    for (i = start_pfn; i + chunk_size <= ALL_PFN; i += chunk_size) {
        struct page *page = &mem_map.pages[i];
        list_add_head(&page->buddy, &Buddy.free_lists[max_order]);
    }

    // 计算剩余页面的起始地址和大小
    uint64 remaining_start = start_pfn + chunk_size * ((ALL_PFN - start_pfn) / chunk_size);
    uint64 remaining_pages = ALL_PFN - remaining_start;

    // 处理剩余页面，按伙伴系统规则分割到低阶
    for (int order = max_order - 1; order >= 0 && remaining_pages > 0; order--) {
        uint64 block_size = 1UL << order;
        // 确保剩余起始地址对齐到当前 order 的块大小
        if (remaining_start % block_size != 0) {
            // 调整起始地址到对齐位置
            remaining_start = (remaining_start / block_size + 1) * block_size;
            remaining_pages = ALL_PFN - remaining_start;
            if (remaining_pages < block_size) {
                continue;  // 当前 order 的块无法分割，跳过
            }
        }
        // 分割剩余页面
        while (remaining_pages >= block_size) {
            struct page *page = &mem_map.pages[remaining_start];
            list_add_head(&page->buddy, &Buddy.free_lists[order]);
            remaining_start += block_size;
            remaining_pages -= block_size;
        }
    }

    __sync_synchronize();
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
