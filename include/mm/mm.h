#ifndef __MM_H__
#define __MM_H__
#include "std/stddef.h"

extern struct page * alloc_pages(uint32 flags, const int order);
extern struct page * alloc_page(uint32 flags);
extern void * __alloc_pages(uint32 flags, const int order);
extern void * __alloc_page(uint32 flags);


extern void free_pages(struct page *pages, const int order);
extern void free_page(struct page *page);
extern void __free_pages(void* addr, const int order);
extern void __free_page(void* addr);


#endif