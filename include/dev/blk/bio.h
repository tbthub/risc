#ifndef __BIO_H__
#define __BIO_H__

#include "std/stddef.h"

struct bio
{
    uint32_t b_blockno; // 块设备开始扇区，一个bio只负责一个块（4K），多余的会被分割
    uint32_t offset;
    uint32_t len;

    struct bio *b_next; // 下一个 bio
    
    void *b_page; // 内存中的数据的地址，也就是缓冲区，是实际的内核页表地址
};

extern struct bio *bio_list_make(uint32_t blockno, uint32_t offset, uint32_t len);
extern void bio_del(struct bio *bio);

#endif
