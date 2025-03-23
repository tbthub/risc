#ifndef __KMALLOC_H__
#define __KMALLOC_H__

#include "mm/slab.h"

extern void *kmalloc(int size, uint32 flags);
extern void kfree(void *obj);

#endif
