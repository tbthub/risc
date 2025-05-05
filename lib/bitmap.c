#include "lib/bitmap.h"
#include "lib/string.h"
#include "std/stdio.h"

void bitmap_init(struct bitmap *bmp, uint64_t *map, int size_bits)
{
	bmp->map = map;
	bmp->size = size_bits;
	bmp->unused = size_bits;
	bmp->allocation = 0;
	memset(map, '\0', (size_bits + 7) / 8);
}

void bitmap_init_zone(struct bitmap *bmp, uint64_t *map, int size_bits)
{
	bmp->map = map;
	bmp->size = size_bits;
	bmp->unused = size_bits;
	bmp->allocation = 0;
}

// 从 `start` 开始归位
inline void bitmap_reset_start(struct bitmap *bmp, const int start)
{
	bmp->allocation = start;
	int index = bitmap_alloc(bmp);
	bitmap_free(bmp, index);
}

// 设置剩余数
inline void bitmap_reset_unused(struct bitmap *bmp, const int _unused)
{
	bmp->unused = _unused;
}

int bitmap_alloc(struct bitmap *bmp)
{
	if (bmp->unused == 0) {
		return -1; // 没有空闲位
	}
	int res, i, bit;
	int total_blocks = (bmp->size + 63) / 64; // 计算总块数

	// 从 `allocation` 开始扫描
	int start_block = bmp->allocation / 64;
	int start_bit = bmp->allocation % 64;

	// 从当前块的 `start_bit` 开始搜索
	for (i = start_block; i < total_blocks; i++) {
		if (bmp->map[i] != ~0ULL) { // 该块存在空闲位
			for (bit = (i == start_block ? start_bit : 0); bit < 64;
			     bit++) {
				// 如果 bit 位为 0
				if (!(bmp->map[i] & (1ULL << bit))) {
					res = i * 64 + bit;
					if (res > bmp->size)
						return -1;
					bmp->map[i] |=
						(1ULL << bit); // 设置该位
					bmp->unused--; // 更新剩余可用位
					bmp->allocation =
						res + 1; // 更新分配位置
					return res; // 返回分配的位索引
				}
			}
		}
	}
	return -1; // 没有空闲位
}

// 采用`铭记`的方式记录，即 allocation 始终为第一个空闲的
// 这样避免每次从头开始扫描
void bitmap_free(struct bitmap *bmp, int index)
{
	if (index < 0 || index >= bmp->size) {
		printk("bitmap_free: index %d out of range\n", index);
		return; // 无效索引
	}

	int map_index = index / 64;
	int bit = index % 64;

	// `铭记`
	if (index < bmp->allocation)
		bmp->allocation = index;

	// 如果 bit 为位 1
	if (bmp->map[map_index] & (1ULL << bit)) {
		bmp->map[map_index] &= ~(1ULL << bit);
		bmp->unused++;
	} else {
		printk("bitmap free: index %d double free!\n", index);
	}
}

int bitmap_is_free(struct bitmap *bmp, int index)
{
	if (index < 0 || index >= bmp->size) {
		return -1;
	}

	int map_index = index / 64;
	int bit = index % 64;

	return !(bmp->map[map_index] & (1ULL << bit));
}

void bitmap_info(struct bitmap *bmp, int show_bits)
{
	printk("  -------- bitmap info start --------\n");
	printk("  Bitmap Information:\n");
	printk("  Total size (bits): %d\n", bmp->size);
	printk("  Unused bits: %d\n", bmp->unused);
	printk("  Next allocation starts at: %d\n", bmp->allocation);

	int total_blocks = (bmp->size + 63) / 64; // 计算总块数

	if (show_bits) {
		printk("  Bitmap blocks:\n");
		for (int i = 0; i < total_blocks; i++) {
			printk("64 bits %d: \t( ", i);
			// 打印每个块的二进制形式
			for (int j = 63; j >= 0; j--) {
				printk("%d", (bmp->map[i] >> j) &
						     1); // 打印每一位的值
				if (j % 8 == 0)
					printk(" "); // 每 8 位添加一个空格
			}
			printk(")\n");
		}
	}
	printk("  -------- bitmap info end --------\n");
}
