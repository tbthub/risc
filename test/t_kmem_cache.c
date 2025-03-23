// #include "mm/slab.h"
// #include "std/stdio.h"
// void kmem_cache_test()
// {

//     // 创建一个 kmem_cache 对象，用于分配大小为 17 字节的对象
//     struct kmem_cache things;
//     kmem_cache_create(&things, "things", 8192, 0);

//     // // 分配多个对象
//     void *a1 = kmem_cache_alloc(&things);
//     void *a2 = kmem_cache_alloc(&things);
//     void *a3 = kmem_cache_alloc(&things);
//     void *a4 = kmem_cache_alloc(&things);
//     void *a5 = kmem_cache_alloc(&things);

//     // // 打印分配的地址
//     printk("Allocated addresses:\n");
//     printk("a1: %p\n", a1);
//     printk("a2: %p\n", a2);
//     printk("a3: %p\n", a3);
//     printk("a4: %p\n", a4);
//     printk("a5: %p\n", a5);

//     // // 释放一个对象
//     kmem_cache_free(&things, a1);
//     printk("Freed a1: %p\n", a1);

//     // // 再次分配一个对象，应该复用 a1 的位置
//     void *a6 = kmem_cache_alloc(&things);
//     printk("a6 (should reuse a1): %p\n", a6);

//     kmem_cache_destory(&things);

//     // // 分配更多对象，测试边界
//     void *a7 = kmem_cache_alloc(&things);
//     // void* a8 = kmem_cache_alloc(&things);

//     printk("a7: %p\n", a7);
//     // printk("a8: %p\n", a8);

//     // // 释放所有对象
//     // kmem_cache_free(&things, a2);
//     // kmem_cache_free(&things, a3);
//     // kmem_cache_free(&things, a4);
//     // kmem_cache_free(&things, a5);
//     // kmem_cache_free(&things, a6);
//     // kmem_cache_free(&things, a7);
//     // kmem_cache_free(&things, a8);

//     // printk("Freed all allocated objects.\n");

//     // // 再次分配，验证是否能正确分配空闲的对象
//     // void* a9 = kmem_cache_alloc(&things);
//     // printk("a9 (should reuse freed space): %p\n", a9);

//     // // 检查边界条件：在没有释放更多内存的情况下继续分配
//     // for (int i = 0; i < 10; i++) {
//     //     void* addr = kmem_cache_alloc(&things);
//     //     if (addr) {
//     //         printk("Allocated object %d: %p\n", i, addr);
//     //     } else {
//     //         printk("Allocation failed at object %d\n", i);
//     //         break;
//     //     }
//     // }
//     // void* addr[406];
//     // for (int i = 0; i < 406; i++)
//     // {
//     //     addr[i] = kmem_cache_alloc(&things);
//     //     printk("a%d: %p\n",i,addr[i]);
//     // }
//     // for (int i = 0; i < 406; i++)
//     // {
//     //     kmem_cache_free(&things,addr[i]);
//     //      printk("free %d\n",i);
//     // }
//     // for (int i = 0; i < 406; i++)
//     // {
//     //     addr[i] = kmem_cache_alloc(&things);
//     //     printk("a%d: %p\n",i,addr[i]);
//     // }
// }
