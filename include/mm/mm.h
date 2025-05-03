#ifndef __MM_H__
#define __MM_H__
#include "std/stddef.h"
#include "riscv.h"

#define MAX_LEVEL 11
#define MAX_LEVEL_INDEX (MAX_LEVEL - 1)        // 最高 10
#define MAX_LEVEL_COUNT (1 << MAX_LEVEL_INDEX) // 最高 2^10个页面
#define MAX_LEVEL_PGSIZE (PGSIZE * (1 << MAX_LEVEL_INDEX)) // 最高 2^10 * 4096 大小

extern struct page * alloc_pages(uint32 flags, const int order);
extern struct page * alloc_page(uint32 flags);
extern void * __alloc_pages(uint32 flags, const int order);
extern void * __alloc_page(uint32 flags);


// 实际上这几个函数两两内部实现都是一毛一样的
// 我们区分不同的函数仅仅是为了可以辨别申请空间的类型
extern void free_pages(struct page *pages);
extern void free_page(struct page *page);
extern void __free_pages(void* addr);
extern void __free_page(void* addr);

extern uint32 get_free_pages();


#endif
