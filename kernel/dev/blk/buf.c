#include "dev/blk/buf.h"
#include "lib/spinlock.h"
#include "lib/hash.h"
#include "lib/atomic.h"
#include "std/stdio.h"
#include "mm/kmalloc.h"
#include "dev/blk/gendisk.h"
#include "core/timer.h"
#include "mm/mm.h"

inline void buf_pin(struct buf_head *b)
{
    sleep_on(&b->lock);
}

inline void buf_unpin(struct buf_head *b)
{
    wake_up(&b->lock);
}

inline void buf_fixed(struct buf_head *b)
{
    SET_FLAG(&b->flags, BH_Fixed);
}

inline int buf_is_fixed(struct buf_head *b)
{
    return TEST_FLAG(&b->flags, BH_Fixed);
}

inline void buf_unfixed(struct buf_head *b)
{
    CLEAR_FLAG(&b->flags, BH_Fixed);
}

static inline void buf_struct_free(struct buf_head *b)
{
    kmem_cache_free(&buf_kmem_cache, b);
}

// 初始化 bhash
void bhash_init(struct bhash_struct *bhash, struct gendisk *gd)
{
    bhash->gd = gd;

    spin_init(&bhash->lock, "bcache");

    hash_init(&bhash->buf_hash_table, 32, "bcache");

    INIT_LIST_HEAD(&bhash->active_list);
    INIT_LIST_HEAD(&bhash->inactive_list);
    bhash->active_count = 0;
    bhash->inactive_count = 0;

    INIT_LIST_HEAD(&bhash->dirty_list);
}

// 块号为 blockno 的哈希链条是否为空
static inline int bhash_empty(struct bhash_struct *bhash, uint32 blockno)
{
    return hash_empty(&bhash->buf_hash_table, blockno);
}

static void bhash_lru(struct bhash_struct *bhash, struct buf_head *b)
{
    // 如果已经在活跃链表中
    if (TEST_FLAG(&b->flags, BH_Active) == BH_Active)
    {
        // list_del(&b->lru);
        // list_add_head(&b->lru, &bhash->active_list);
    }
    // 如果是新的第二次访问，则要从当前 lru 移除，加入到 active_list 头
    else if (TEST_FLAG(&b->flags, BH_Visited) == BH_Visited)
    {
        SET_FLAG(&b->flags, BH_Active);
        list_del(&b->lru);
        list_add_head(&b->lru, &bhash->active_list);
        bhash->active_count++;
    }
    // 新创建 BH_Visited 为0的，加入 inactive_list，并设为访问过一次
    else
    {
        SET_FLAG(&b->flags, BH_Visited);
        list_add_head(&b->lru, &bhash->inactive_list);
        bhash->inactive_count++;
    }
}

// 将缓冲区加入哈希表
static inline void bhash_add(struct bhash_struct *bhash, struct buf_head *b)
{
    // 插入哈希表
    hash_add_head(&bhash->buf_hash_table, b->blockno, &b->bh_node);
}

// 将缓冲区从哈希表中摘下
static inline void bhash_del(struct bhash_struct *bhash, struct buf_head *b)
{
    hash_del_node(&bhash->buf_hash_table, &b->bh_node);
}

// 在哈希表中查找特定的缓存块
static struct buf_head *bhash_find(struct bhash_struct *bhash, uint _blockno)
{
    struct buf_head *b;

    hash_find(b, &bhash->buf_hash_table, blockno, _blockno, bh_node);
    if (b)
    {
        bhash_lru(bhash, b);
        // b->refcnt++;
    }

    return b;
}

// 尤其注意，获取前需要调用 bhash_find 查找
// 这个是专门用于申请新 buf_head 的
// 内存中绝不允许存在两个一样块号的缓存
static struct buf_head *buf_alloc(struct gendisk *gd, uint blockno)
{
    // struct bhash_struct *bhash = &gd->bhash;
    struct buf_head *b = (struct buf_head *)kmem_cache_alloc(&buf_kmem_cache);
    if (!b)
        panic("buf.c - bget - kmem_cache_alloc failed");

    b->gd = gd;
    b->blockno = blockno;
    sleep_init(&b->lock, "buf_head");
    atomic_set(&b->refcnt, 0);
    INIT_HASH_NODE(&b->bh_node);
    INIT_LIST_HEAD(&b->lru);
    INIT_LIST_HEAD(&b->dirty);

    SET_FLAG(&b->flags, BH_New);

    b->page = __alloc_page(0);
    if (!b->page)
        panic("buf_alloc");

    return b;
}

struct buf_head *buf_get(struct gendisk *gd, uint blockno)
{
    struct buf_head *buf;
    // int creating = 0;
    spin_lock(&gd->bhash.lock);
    buf = bhash_find(&gd->bhash, blockno);
    if (!buf)
    {
        buf = buf_alloc(gd, blockno);
        bhash_add(&gd->bhash, buf);
        bhash_lru(&gd->bhash, buf);
        // 只是为了挡住其他查找到 buf
        // sleep_on(&buf->lock);

        // creating = 1;
    }
    spin_unlock(&gd->bhash.lock);
    // if (!TEST_FLAG(&buf->flags, BH_Valid))
    // {
    //     sleep_on(&buf->lock);
    //     wake_up(&buf->lock);
    // }
    // if (creating)
    // {
    //     SET_FLAG(&buf->flags, BH_Valid);
    //     wake_up(&buf->lock);
    // }
    atomic_inc(&buf->refcnt);
    return buf;
}

// 仅仅打个脏标记，减少计数,放入脏链。以后由内核线程清理掉不用的，和 LRU 没关系
void buf_release(struct buf_head *b, int is_dirty)
{
    if (!b)
        return;
    // 如果缓存被锁定，不允许释放，此时那个进程应该持有睡眠锁
    // 需要等到睡眠锁释放后才可以，于是这里陷入等待
    // 因为 (find、get) | release 配对，在 relse 前会增加引用计数，如果能释放的情况下，不会被 flush 释放
    // buf_pin(b);
    atomic_dec(&b->refcnt);
    buf_used(b);

    // printk("buf_release: %p\n",b->flags);

    // 正常使用过的缓存,加入脏链
    if (is_dirty)
    {
        // printk("buf_release dirty to list\n");

        SET_FLAG(&b->flags, BH_Dirty);
        spin_lock(&b->gd->bhash.lock);
        if (!list_empty(&b->gd->bhash.dirty_list))
            list_del(&b->dirty);
        list_add_head(&b->dirty, &b->gd->bhash.dirty_list);

        spin_unlock(&b->gd->bhash.lock);
    }
    // buf_unpin(b);
}

// 从活跃移动到不活跃的,感觉这个函数最好不用，太慢了
// static void buf_act2ina(struct bhash_struct *bhash, struct buf_head *b)
// {
//     buf_pin(b);
//     spin_lock(&bhash->lock);

//     list_del(&b->lru);
//     list_add_head(&b->lru, &bhash->inactive_list);
//     bhash->active_num--;
//     bhash->inactive_num++;
//     CLEAR_FLAG(&b->flags, BH_Active);

//     spin_unlock(&bhash->lock);
//     buf_unpin(b);
// }

// ------------关于缓存一致性问题的讨论------------