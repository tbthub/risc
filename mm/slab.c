// #include "mm/mm.h"
// #include "mm/page.h"
// #include "mm/slab.h"

// #include "lib/math.h"
// #include "lib/string.h"

// #include "core/proc.h"
// #include "core/timer.h"

// #include "dev/blk/buf.h"
// #include "dev/blk/bio.h"

// #include "../fs/easyfs/easyfs.h"

// #include "fs/file.h"

// #include "param.h"

// extern struct slab *page_slab(struct page *page);
// extern void set_page_slab(struct page *page, struct slab *slab);
// extern void clear_page_slab(struct page *page);

// struct list_head kmem_cache_list;

// // 专用缓存
// struct kmem_cache task_struct_kmem_cache;
// struct kmem_cache thread_info_kmem_cache;
// struct kmem_cache buf_kmem_cache;
// struct kmem_cache bio_kmem_cache;
// struct kmem_cache timer_kmem_cache;
// struct kmem_cache efs_inode_kmem_cache;
// struct kmem_cache efs_dentry_kmem_cache;
// struct kmem_cache file_kmem_cache;
// struct kmem_cache tf_kmem_cache;
// struct kmem_cache vma_kmem_cache;

// // 由 kmem_cache 信息创建一个 slab 节点（未插入 kmem_cache.slabs 链表）
// // 注：kmem_cache.size 需要已经对齐
// static struct slab *slab_create(struct kmem_cache *cache)
// {
// 	int i, page_count;
// 	struct slab *slab;
// 	struct page *page;

// 	slab = (struct slab *)__alloc_page(cache->flags);
// 	if (!slab)
// 		return NULL;
// 	slab->objs = __alloc_pages(cache->flags, cache->order);
// 	if (!slab->objs) {
// 		// 释放已分配的 slab
// 		__free_page((void *)slab);
// 		return NULL;
// 	}

// 	// 修改页面标志
// 	page = PA2PG(slab->objs);
// 	page_count = 1 << cache->order;
// 	for (i = 0; i < page_count; i++)
// 		set_page_slab(page + i, slab);

// 	slab->kc = cache;
// 	slab->inuse = 0;
// 	slab->free_list = &(slab->free_list);
// 	INIT_LIST_HEAD(&slab->list);
// 	for (uint i = 0; i < cache->count_per_slab; i++) {
// 		*((uint64 *)slab->free_list + i + 1) = (uint64)((char *)slab->objs + i * cache->size);
// 	}
// 	slab->free_list = (uint64 *)slab->free_list + 1;
// 	return slab;
// }

// static void slab_destory(struct slab *slab)
// {
// 	int i, page_count;
// 	struct page *page;

// 	// 恢复页面
// 	page = PA2PG(slab->objs);
// 	page_count = 1 << slab->kc->order;
// 	for (i = 0; i < page_count; i++)
// 		clear_page_slab(page + i);

// 	// 释放页面
// 	__free_pages((void *)slab->objs, slab->kc->order);
// 	__free_page((void *)slab);
// }

// // 计算满足最少容纳 MIN_OBJ_COUNT_PER_PAGE 个对象的order
// static uint8 calc_slab_order(uint16 obj_size)
// {
// 	// 确保每个 slab 至少容纳 MIN_OBJ_COUNT_PER_PAGE 个对象
// 	uint32 require_size = obj_size * MIN_OBJ_COUNT_PER_PAGE;

// 	uint8 order = 0;
// 	uint32 slab_size = PGSIZE;

// 	while (require_size > slab_size) {
// 		order++;
// 		slab_size <<= 1;
// 	}

// 	return order;
// }

// // 弹出一个空闲的地址（申请地址）
// static void *obj_pop(struct slab *slab)
// {
// 	slab->inuse++;
// 	void *tmp = (void *)(*(uint64 *)(slab->free_list));
// 	// 指针上移
// 	slab->free_list = (uint64 *)slab->free_list + 1;

// 	return tmp;
// }

// // 插入一个空闲的地址（释放地址）
// static void obj_push(struct slab *slab, void *obj)
// {
// 	slab->inuse--;
// 	// 指针下移
// 	slab->free_list = (uint64 *)slab->free_list - 1;
// 	*(void **)slab->free_list = obj;
// }

// // slab 是不是未满，还有空
// static int is_slab_partial(struct slab *slab, struct kmem_cache *cache)
// {
// 	return slab->inuse < cache->count_per_slab;
// }

// // 初始化每个 kmem_cache 缓存的 cache_cpu 部分
// // 注：kmem_cache.size 需要已经对齐
// static void cache_cpu_init(struct kmem_cache *cache)
// {
// 	uint16 i;
// 	struct slab *cpu_slab;
// 	for (i = 0; i < NCPU; i++) {
// 		cpu_slab = slab_create(cache);
// 		if (!cpu_slab)
// 			panic("slab.c: cache_cpu_init Faild!\n");
// 		cache->cache_cpu[i] = cpu_slab;
// 	}
// }

// // 增加 slab
// static struct slab *kmem_cache_add_slab(struct kmem_cache *cache)
// {
// 	struct slab *new_slab = slab_create(cache);
// 	list_add_head(&new_slab->list, &cache->part_slabs);
// 	return new_slab;
// }

// // 初始化缓存池
// void kmem_cache_create(struct kmem_cache *cache, const char *name, uint16 size, uint32 flags)
// {
// 	spin_init(&cache->lock, "slab");

// 	strncpy(cache->name, name, CACHE_MAX_NAME_LEN);
// 	cache->flags = flags;
// 	cache->size = (uint16)next_power_of_2(size); // 对齐
// 	cache->order = calc_slab_order(cache->size);
// 	cache->count_per_slab = (1 << cache->order) * PGSIZE / cache->size;
// 	INIT_LIST_HEAD(&cache->part_slabs);
// 	INIT_LIST_HEAD(&cache->full_slabs);
// 	INIT_LIST_HEAD(&cache->list);
// 	cache_cpu_init(cache);

// 	list_add_head(&cache->list, &kmem_cache_list);
// }

// // 这里暂时还有一点问题
// // 考虑到直接静态分配，因此感觉这里只释放了资源，并没有释放结构本身
// void kmem_cache_destory(struct kmem_cache *cache)
// {
// 	if (!cache)
// 		panic("kmem_cache_destory: does not exist! Due to NULL!\n");

// 	int i;
// 	struct slab *slab, *tmp;

// 	spin_lock(&cache->lock);
// 	// 销毁 cpu_cache
// 	for (i = 0; i < NCPU; i++)
// 		slab_destory(cache->cache_cpu[i]);

// 	// 销毁slabs
// 	list_for_each_entry_safe(slab, tmp, &cache->part_slabs, list) {
// 		slab_destory(slab);
// 	}
// 	list_for_each_entry_safe(slab, tmp, &cache->full_slabs, list) {
// 		slab_destory(slab);
// 	}
// 	list_del_init(&cache->list);
// 	spin_unlock(&cache->lock);
// 	printk("destory kmem_cache: '%s' ok!\n", cache->name);
// }

// // 申请对象
// void *kmem_cache_alloc(struct kmem_cache *cache)
// {
// 	if (!cache)
// 		panic("kmem_cache_alloc: does not exist! Due to NULL!\n");

// 	if (list_empty(&cache->list))
// 		panic("kmem_cache_alloc: '%s' has already been freed!\n", cache->name);

// 	struct slab *slab;
// 	struct slab *cpu_slab;
// 	void *addr;
	
// 	push_off();
// 	cpu_slab = cache->cache_cpu[cpuid()];
// 	pop_off();

// 	// 如果当前还有可用的，直接弹出即可
// 	if (is_slab_partial(cpu_slab, cache)) {
// 		return obj_pop(cpu_slab);
// 	}

// 	// 如果 cpu_cache 可用的已经分配完了
// 	// 则从 part_slabs 链中查找
// 	spin_lock(&cache->lock);
// 	// 如果 part_slabs 为空，则说明现在已经没有空闲的slab可用了，于是新添一个
// 	// 新添的这个第一次分配一定还有剩余的，弹出一个地址返回即可
// 	if (list_empty(&cache->part_slabs)) {
// 		slab = kmem_cache_add_slab(cache);
// 		addr = obj_pop(slab);
// 		spin_unlock(&cache->lock);
// 		return addr;
// 	}

// 	// 一般情况下，从当前的 part_slabs 中查找
// 	// 当前的还有可用，这里需要看是不是分出去后就没有了
// 	slab = list_entry(list_first(&cache->part_slabs), struct slab, list);
// 	addr = obj_pop(slab);
// 	if (!is_slab_partial(slab, cache)) {
// 		list_del(&slab->list);
// 		list_add_head(&slab->list, &cache->full_slabs);
// 	}

// 	spin_unlock(&cache->lock);
// 	return addr;
// }

// // 释放对象
// void kmem_cache_free(struct kmem_cache *cache, void *obj)
// {
// 	if (!cache)
// 		panic("kmem_cache_free: does not exist! Due to NULL!\n");

// 	if (list_empty(&cache->list))
// 		panic("kmem_cache_free: '%s' has already been freed!\n", cache->name);

// 	struct slab *slab = page_slab(PA2PG(obj));

// 	// 如果链表为空，说明是 cpu_cache,直接释放即可
// 	// 如果是 cpu_cache,则其他cpu无法访问这个slab的，是安全的，不必加锁
// 	if (list_empty(&slab->list)) {
// 		obj_push(slab, obj);
// 		return;
// 	}

// 	// 否则就是普通的
// 	spin_lock(&cache->lock);

// 	// 如果原先全部都分配出去了，要 full 里提取出来放到 part 上
// 	if (!is_slab_partial(slab, cache)) {
// 		list_del(&slab->list);
// 		list_add_head(&slab->list, &cache->part_slabs);
// 	}
// 	obj_push(slab, obj);

// 	spin_unlock(&cache->lock);
// }

// // 初始化 Slab 分配器
// void kmem_cache_init()
// {
// 	INIT_LIST_HEAD(&kmem_cache_list);

// 	// 初始化专用缓存
// 	kmem_cache_create(&task_struct_kmem_cache, "task_struct_kmem_cache", sizeof(struct task_struct), 0);
// 	kmem_cache_create(&thread_info_kmem_cache, "thread_info_kmem_cache", 2 * PGSIZE, 0);
// 	kmem_cache_create(&buf_kmem_cache, "buf_kmem_cache", sizeof(struct buf_head), 0);
// 	kmem_cache_create(&bio_kmem_cache, "bio_kmem_cache", sizeof(struct bio), 0);
// 	kmem_cache_create(&timer_kmem_cache, "timer_kmem_cache", sizeof(struct timer), 0);
// 	kmem_cache_create(&efs_inode_kmem_cache, "inode_kmem_cache", sizeof(struct easy_m_inode), 0);
// 	kmem_cache_create(&efs_dentry_kmem_cache, "dentry_kmem_cache", sizeof(struct easy_dentry), 0);
// 	kmem_cache_create(&file_kmem_cache, "file_kmem_cache", sizeof(struct file), 0);
// 	kmem_cache_create(&tf_kmem_cache, "tf_kmem_cache", sizeof(struct trapframe), 0);
// 	kmem_cache_create(&vma_kmem_cache, "vma_kmem_cache", sizeof(struct vm_area_struct), 0);
// }
