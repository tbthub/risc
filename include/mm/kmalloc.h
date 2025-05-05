#ifndef __KMALLOC_H__
#define __KMALLOC_H__

#include "mm/slab.h"
#include "mm/mm.h"

#define MAX_KMALLOC_SLAB 8192
#define MAX_KMALLOC_PAGE MAX_LEVEL_PGSIZE

extern void *kmalloc(int size, uint32_t flags);
extern void kfree(void *obj);

#endif
