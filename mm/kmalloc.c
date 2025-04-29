#include "mm/slab.h"
#include "mm/page.h"

#include "std/string.h"
#include "lib/math.h"

extern struct slab *page_slab(struct page *page);

#define MAX_KMALLOC 8192

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
    [1] & kmalloc32,
    [2] & kmalloc64,
    [3] & kmalloc128,
    [4] & kmalloc256,
    [5] & kmalloc512,
    [6] & kmalloc1024,
    [7] & kmalloc2048,
    [8] & kmalloc4096,
    [9] & kmalloc8192,
};

void kmalloc_init() {
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

void *kmalloc(int size, uint32 flags) {
    if (size < 1 || size > MAX_KMALLOC) {
        panic("!!! kmalloc: size illegal, must be between 1 and %d !!!\n",
              MAX_KMALLOC);
        return NULL;
    }
    uint32 order = 0;
    if (size < 16)
        order = 0;
    else
        order = calculate_order((uint32)size) - 4;
    void *addr = kmem_cache_alloc(kmalloc_caches[order]);

    if (!addr)
        panic("kmalloc: Allocation failed for size %d.\n", size);

    memset(addr, 0, size);
    return addr;
}

void kfree(void *obj) {
    struct slab *slab;
    slab = page_slab(PA2PG(obj));

    kmem_cache_free(slab->kc, obj);
}
