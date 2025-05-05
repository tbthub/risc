#ifndef __BITMAP_H__
#define __BITMAP_H__
#include "std/stddef.h"

// 以后我们采取一簇一簇对位图分配进行优化

struct bitmap {
	uint64_t *map; // 实际的物理指针
	int size; // 位图的大小
	int unused; // 剩余可用的

	// 下一次线性搜索开始的位置，当有节点被分配出去或者回收的时候会改变这个
	// 避免每次都从头开始
	int allocation;
};

extern void bitmap_init(struct bitmap *bmp, uint64_t *map, int size_bits);
extern void bitmap_init_zone(struct bitmap *bmp, uint64_t *map, int size_bits);
extern int bitmap_alloc(struct bitmap *bmp);
extern void bitmap_free(struct bitmap *bmp, int index);
extern int bitmap_is_free(struct bitmap *bmp, int index);
extern void bitmap_info(struct bitmap *bmp, int show_bits);

extern void bitmap_reset_start(struct bitmap *bmp, const int start);
extern void bitmap_reset_unused(struct bitmap *bmp, const int _unused);

#endif
