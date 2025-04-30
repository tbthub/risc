
#include "riscv.h"
#include "mm/mm.h"
#include "mm/kmalloc.h"

#include "dev/blk/bio.h"
#define BLK_SIZE 4096

static struct bio *bio_alloc(uint32 blockno, uint32 offset, uint32 len, struct bio *next, void *page)
{
    struct bio *b = (struct bio *)kmem_cache_alloc(&bio_kmem_cache);
    if (!b)
        panic("bio_make\n");
    b->b_blockno = blockno;
    b->offset = offset;
    b->len = len;
    b->b_next = next;
    b->b_page = page;
    return b;
}

// 这里需要注意插入顺序
// 这个链表需要反过来，也就是说，需要尾插。头插顺序会反。
// 但是为了插入方便(头插简单)，我们把 blockno 倒过来赋值，即 --blockno_top

// 这里返回的链表没有链表头的
struct bio *bio_list_make(uint32 blockno, uint32 offset, uint32 len)
{
    if (len == 0)
        return NULL;
    struct bio *bio_list = NULL; // 使用动态分配来保存链表头
    struct bio *b;
    uint32 count;

    // 预处理, 避免 offset 超过一个块大小导致实际上有块没必要读
    blockno += offset / BLK_SIZE;
    offset = offset % BLK_SIZE;

    count = (offset + len - 1) / BLK_SIZE + 1; // 需要读出的块数
    if (count == 1)
    {
        b = bio_alloc(blockno, offset, len, bio_list, NULL);
        bio_list = b;
    }
    else if (count > 1)
    {
        blockno += count;

        // 第 1 个
        b = bio_alloc(--blockno, 0, ((offset + len - 1) % BLK_SIZE) + 1, bio_list, NULL);
        bio_list = b; // 更新链表头
        count--;

        // 处理中间的块
        while (count > 1)
        {
            b = bio_alloc(--blockno, 0, BLK_SIZE, bio_list, NULL); // 中间块，偏移为 0，长度为完整块
            bio_list = b;
            count--;
        }

        // 最后一个
        b = bio_alloc(--blockno, offset, BLK_SIZE - offset, bio_list, NULL); // 最后一个块
        bio_list = b;
    }
    else
    {
        panic("bio_list_make NONE!\n");
    }

    return bio_list; // 返回新的链表头
}

inline void bio_del(struct bio *bio)
{
    kmem_cache_free(&bio_kmem_cache, bio);
}