#include "lib/bitmap.h"
#include "mm/kmalloc.h"

int bitmap_test()
{
    printk("Bitmap Test start.\n");

    uint64 map[2]; // 128 位 (2 个 uint64)，更大位图
    struct bitmap bmp;

    bitmap_init(&bmp, map, 128);

    printk("---- Allocating Test ----\n");

    // 分配测试
    for (int i = 0; i < 128; i++)
    {
        printk("Allocating index: %d\n", bitmap_alloc(&bmp)); // 分配 128 位中的每一位
    }

    // 检查空闲状态
    printk("Is index 5 free? %d\n", bitmap_is_free(&bmp, 5));     // 应该为 0（不空闲）
    printk("Is index 100 free? %d\n", bitmap_is_free(&bmp, 100)); // 应该为 0（不空闲）
    printk("Is index 128 free? %d\n", bitmap_is_free(&bmp, 128)); // 应该为 -1（越界）

    // 试图分配更多空间，应该返回 -1
    printk("Allocating after full bitmap: %d\n", bitmap_alloc(&bmp)); // 应该为 -1

    printk("---- Freeing Test ----\n");

    // 释放位置 10 和 50
    bitmap_free(&bmp, 10);
    bitmap_free(&bmp, 50);

    // 检查释放后的状态
    printk("Is index 10 free? %d\n", bitmap_is_free(&bmp, 10)); // 应该为 1（空闲）
    printk("Is index 50 free? %d\n", bitmap_is_free(&bmp, 50)); // 应该为 1（空闲）

    // 重新分配位置 10 和 50
    printk("Allocating after freeing 10: %d\n", bitmap_alloc(&bmp)); // 应该为 10
    printk("Allocating after freeing 50: %d\n", bitmap_alloc(&bmp)); // 应该为 50

    printk("---- Edge Case Test ----\n");

    // 检查第一个和最后一个位置
    bitmap_free(&bmp, 0);
    bitmap_free(&bmp, 127);

    printk("Is index 0 free? %d\n", bitmap_is_free(&bmp, 0));     // 应该为 1（空闲）
    printk("Is index 127 free? %d\n", bitmap_is_free(&bmp, 127)); // 应该为 1（空闲）

    // 重新分配第一个和最后一个位置
    printk("Allocating first index: %d\n", bitmap_alloc(&bmp)); // 应该为 0
    printk("Allocating last index: %d\n", bitmap_alloc(&bmp));  // 应该为 127

    printk("---- Random Allocation and Free Test ----\n");
    bitmap_info(&bmp,0);
    // 随机分配和释放位置
    for (int i = 0; i < 10; i++)
    {
        bitmap_free(&bmp, i);
        int allocated = bitmap_alloc(&bmp);
        printk("Random Allocating index: %d\n", allocated);
        if (allocated >= 0 && allocated < 128)
        {
            bitmap_free(&bmp, allocated); // 随后释放
            printk("Random Freeing index: %d\n", allocated);
        }
    }

    bitmap_info(&bmp,0);

    // 继续分配，确保位图继续工作
    for (int i = 0; i < 11; i++)
    {
        printk("Allocating after random frees: %d\n", bitmap_alloc(&bmp)); // 应该能继续分配
    }

    printk("---- Edge Case - Out of Bound Test ----\n");
    // 尝试越界操作
    printk("Allocating out of bound: %d\n", bitmap_alloc(&bmp)); // 应该返回 -1
    bitmap_free(&bmp, 128);                                      // 越界释放，应当无效

    // 分配后测试越界访问
    printk("Is index 128 free (out of bound)? %d\n", bitmap_is_free(&bmp, 128)); // 应该返回 -1（越界）

    printk("---- Test after full bitmap reset ----\n");

    // 逐个释放所有位置
    for (int i = 0; i < 128; i++)
    {
        bitmap_free(&bmp, i);
    }

    // 检查位图恢复
    for (int i = 0; i < 128; i++)
    {
        printk("Allocating after full reset: %d\n", bitmap_alloc(&bmp)); // 应该依次返回 0, 1, 2, ..., 127
    }

    printk("---- Performance Test ----\n");

    // 性能测试：快速分配和释放
    for (int i = 0; i < 128; i++)
    {
        bitmap_alloc(&bmp);
    }

    for (int i = 0; i < 128; i++)
    {
        bitmap_free(&bmp, i);
    }
    bitmap_free(&bmp, 10);

    printk("Bitmap Test complete.\n");

    return 0;
}
