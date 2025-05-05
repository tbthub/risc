#include "mm/kmalloc.h"

#include "core/export.h"
#include "lib/math.h"
#include "lib/string.h"
#include "mm/page.h"
#include "mm/slab.h"

extern void set_page_kmalloc_page(struct page *page);
extern uint32_t test_page_kmalloc_page(struct page *page);
extern void clear_page_kmalloc_page(struct page *page);

static struct kmem_cache kmalloc16;
static struct kmem_cache kmalloc32;
static struct kmem_cache kmalloc64;
static struct kmem_cache kmalloc128;
static struct kmem_cache kmalloc256;
static struct kmem_cache kmalloc512;
static struct kmem_cache kmalloc1024;
static struct kmem_cache kmalloc2048;
static struct kmem_cache kmalloc4096;
static struct kmem_cache kmalloc8192;

static struct kmem_cache *kmalloc_caches[] = {
        [0] & kmalloc16,
        [1] & kmalloc16,
        [2] & kmalloc16,
        [3] & kmalloc16,
        [4] & kmalloc16,
        [5] & kmalloc32,
        [6] & kmalloc64,
        [7] & kmalloc128,
        [8] & kmalloc256,
        [9] & kmalloc512,
        [10] & kmalloc1024,
        [11] & kmalloc2048,
        [12] & kmalloc4096,
        [13] & kmalloc8192,
};

void kmalloc_init()
{
    kmem_cache_create(&kmalloc16, "kmalloc-16", 16, 0);
    kmem_cache_create(&kmalloc32, "kmalloc-32", 32, 0);
    kmem_cache_create(&kmalloc64, "kmalloc-64", 64, 0);
    kmem_cache_create(&kmalloc128, "kmalloc-128", 128, 0);
    kmem_cache_create(&kmalloc256, "kmalloc-256", 256, 0);
    kmem_cache_create(&kmalloc512, "kmalloc-512", 512, 0);
    kmem_cache_create(&kmalloc1024, "kmalloc-1024", 1024, 0);
    kmem_cache_create(&kmalloc2048, "kmalloc-2048", 2048, 0);
    kmem_cache_create(&kmalloc4096, "kmalloc-4096", 4096, 0);
    kmem_cache_create(&kmalloc8192, "kmalloc-8192", 8192, 0);
}

void *kmalloc(int size, uint32_t flags)
{
    if (size < 1 || size > MAX_KMALLOC_PAGE) {
        panic("!!! kmalloc: size illegal, must be between 1 and %d !!!\n", MAX_KMALLOC_PAGE);
        return NULL;
    }
    void *addr;
    uint32_t order = 0;
    uint32_t pages;

    if (size <= MAX_KMALLOC_SLAB) {
        order = math_order2((uint32_t)size);
        addr = kmem_cache_alloc(kmalloc_caches[order]);
    }

    else if (size <= MAX_KMALLOC_PAGE) {
        pages = PGROUNDUP(size) / PGSIZE;
        order = math_order2(pages);
        addr = __alloc_pages(0, order);
        set_page_kmalloc_page(PA2PG(addr));
    }
    else {
        // 理论上不会有这个情况
        panic("kmalloc\n");
    }

    if (!addr)
        panic("kmalloc: Allocation failed for size %d.\n", size);

    memset(addr, 0, size);
    return addr;
}
EXPORT_SYMBOL(kmalloc);

void kfree(void *obj)
{
    if (!obj) {
        // for debug
        panic("kfree\n");
    }
    struct page *pg = PA2PG(obj);
    if (test_page_kmalloc_page(pg) == 0) {
        kmem_cache_free(obj);
    }
    else {
        clear_page_kmalloc_page(pg);
        __free_pages(obj);
    }
}
EXPORT_SYMBOL(kfree);
