// #include "mm/mm.h"
// #include "mm/buddy.h"
// #include "std/stdio.h"
// extern struct buddy_struct buddy;

// static void debug()
// {
//     for (int i = 0; i < MAX_LEVEL; i++)
//     {
//         uint len = list_len(&buddy.free_lists[i]);
//         printk("{ L: %d, N: %d }\t", i, len);
//     }
//     printk("\n");
// }

// void mm_test()
// {
//     // Step 1: 初始化内存管理系统
//     mm_init();
//     printk("Initial memory state:\n");
//     debug();
//     printk("-------------OK\n");

//     // Step 2: 边界测试 - 单页分配和释放
//     printk("Test Case 1: Single page allocation and deallocation\n");
//     debug();
//     struct page *addr1 = alloc_page(0); // 分配 1 页
//     debug();
//     free_page(addr1); // 释放
//     debug();
//     printk("-------------OK\n");

//     // Step 3: 多页分配（2^n 页）测试
//     printk("Test Case 2: Allocating and freeing multiple pages (order = 3)\n");
//     debug();
//     struct page *addr2 = alloc_pages(0, 3);
//     debug();
//     free_pages(addr2, 3);
//     debug();
//     printk("-------------OK\n");

//     // Step 4: 非对齐分配（非 2^n 页）模拟
//     printk("Test Case 3: Simulating non-power-of-2 allocation by splitting and combining\n");
//     debug();
//     struct page *addr3a = alloc_page(0);
//     debug();
//     struct page *addr3b = alloc_pages(0, 2);
//     debug();
//     free_page(addr3a);
//     debug();
//     free_pages(addr3b, 2);
//     debug();
//     printk("-------------OK\n");

//     // Step 5: 连续分配多次，释放时乱序
//     printk("Test Case 4: Consecutive allocation and random-order freeing\n");
//     struct page *addr4a = alloc_pages(0, 4); // 分配 16 页 (2^4)
//     struct page *addr4b = alloc_pages(0, 3); // 分配 8 页 (2^3)
//     struct page *addr4c = alloc_page(0);     // 分配 1 页
//     debug();
//     free_pages(addr4b, 3); // 乱序释放 8 页
//     debug();
//     free_pages(addr4a, 4); // 乱序释放 16 页
//     debug();
//     free_page(addr4c); // 乱序释放 1 页
//     debug();
//     printk("-------------OK\n");

//     // Step 6: 分配大块内存（接近总内存大小）
//     printk("Test Case 5: Allocating almost all memory (largest block)\n");
//     struct page *addr5 = alloc_pages(0, MAX_LEVEL - 1); // 分配最大块
//     debug();
//     free_pages(addr5, MAX_LEVEL - 1); // 释放最大块
//     debug();
//     printk("-------------OK\n");

//     // // Step 7: 多级合并测试
//     printk("Test Case 6: Testing multi-level merging\n");
//     struct page *addr6a = alloc_pages(0, 2); // 分配 4 页 (2^2)
//     debug();
//     struct page *addr6b = alloc_pages(0, 2); // 分配 4 页
//     debug();
//     struct page *addr6c = alloc_pages(0, 2); // 分配 4 页
//     debug();
//     free_pages(addr6a, 2); // 释放第一个块
//     free_pages(addr6b, 2); // 释放第二个块，触发合并
//     free_pages(addr6c, 2); // 释放第三个块，触发多级合并
//     debug();
//     printk("-------------OK\n");

//     printk(" :-) \n");
// }