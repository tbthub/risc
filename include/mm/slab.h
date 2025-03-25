#ifndef __SLAB_H__
#define __SLAB_H__
#include "lib/list.h"
#include "lib/spinlock.h"
#include "param.h"
#include "lib/stack.h"

#define CACHE_MAX_NAME_LEN 24
#define FREE_LIST_MAX_LEN (PGSIZE / 16)
#define MIN_OBJ_COUNT_PER_PAGE 4

struct slab;

struct kmem_cache
{
    struct slab *cache_cpu[NCPU];

    spinlock_t lock;

    char name[CACHE_MAX_NAME_LEN];
    uint32 flags;
    uint16 size;
    uint8 order;
    uint16 count_per_slab;

    struct list_head part_slabs;
    struct list_head full_slabs;
    struct list_head list;
};

// 占用一个页面
struct slab
{
    struct kmem_cache *kc;
    void *objs;
    uint16 inuse;
    struct list_head list;
    sstack_t free_list;
    spinlock_t lock;
    // void *free_list;
};

extern struct kmem_cache task_struct_kmem_cache;
extern struct kmem_cache thread_info_kmem_cache;
extern struct kmem_cache buf_kmem_cache;
extern struct kmem_cache bio_kmem_cache;
extern struct kmem_cache timer_kmem_cache;
extern struct kmem_cache efs_inode_kmem_cache;
extern struct kmem_cache efs_dentry_kmem_cache;
extern struct kmem_cache file_kmem_cache;
extern struct kmem_cache tf_kmem_cache;
extern struct kmem_cache vma_kmem_cache;

// 初始化全局内核缓存
extern void kmem_cache_init();

extern void kmem_cache_create(struct kmem_cache *cache, const char *name, uint16 size, uint32 flags);
extern void kmem_cache_destory(struct kmem_cache *cache);
extern void *kmem_cache_alloc(struct kmem_cache *cache);
extern void kmem_cache_free(struct kmem_cache *cache, void *obj);

#endif
