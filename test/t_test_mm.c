// #include "std/string.h"
// #include "mm/kmalloc.h"
// #include "mm/mm.h"
// #include "mm/slab.h"

// #define PAGE_CNT 3000

// #define SLAB_TASK_CNT 1000
// #define SLAB_TINFO_CNT 1000
// #define SLAB_BUF_CNT 1000
// #define SLAB_TIMER_CNT 1000
// #define SLAB_INDDE_CNT 1000
// #define SLAB_DENTRY_CNT 1000
// #define SLAB_FILE_CNT 1000
// #define SLAB_TF_CNT 1000
// #define SLAB_VMA_CNT 1000

// #define KMALLOC16 300
// #define KMALLOC32 300
// #define KMALLOC64 300
// #define KMALLOC128 300
// #define KMALLOC256 300
// #define KMALLOC512 300
// #define KMALLOC1024 300
// #define KMALLOC2048 300
// #define KMALLOC4096 300
// #define KMALLOC8192 300

/*
// #define ALL                                                                                                                                                                                            \
//     (PAGE_CNT + SLAB_TASK_CNT + SLAB_TINFO_CNT + SLAB_BUF_CNT + SLAB_TIMER_CNT + SLAB_INDDE_CNT + SLAB_DENTRY_CNT + SLAB_FILE_CNT + SLAB_TF_CNT + SLAB_VMA_CNT + KMALLOC16 + KMALLOC32 + KMALLOC64 +   \
//      KMALLOC128 + KMALLOC256 + KMALLOC512 + KMALLOC1024 + KMALLOC2048 + KMALLOC4096 + KMALLOC8192)

// #define SLAB_ALL                                                                                                                                                                                       \
//     (SLAB_TASK_CNT + SLAB_TINFO_CNT + SLAB_BUF_CNT + SLAB_TIMER_CNT + SLAB_INDDE_CNT + SLAB_DENTRY_CNT + SLAB_FILE_CNT + SLAB_TF_CNT + SLAB_VMA_CNT + KMALLOC16 + KMALLOC32 + KMALLOC64 + KMALLOC128 + \
//      KMALLOC256 + KMALLOC512 + KMALLOC1024 + KMALLOC2048 + KMALLOC4096 + KMALLOC8192)
*/
// static uint64 *mem_test[ALL];

// extern struct kmem_cache task_struct_kmem_cache;
// extern struct kmem_cache thread_info_kmem_cache;
// extern struct kmem_cache buf_kmem_cache;
// extern struct kmem_cache timer_kmem_cache;
// extern struct kmem_cache efs_inode_kmem_cache;
// extern struct kmem_cache efs_dentry_kmem_cache;
// extern struct kmem_cache file_kmem_cache;
// extern struct kmem_cache tf_kmem_cache;
// extern struct kmem_cache vma_kmem_cache;

// extern void *kmalloc(int size, uint32 flags);
// extern void kfree(void *obj);
// extern void mm_debug();
// static int ele_exist(uint64 *p)
// {
//     if (p == NULL) {
//         panic("P == NULL!");
//     }
//     for (int i = 0; i < ALL; i++) {
//         if (mem_test[i] == p)
//             return 1;
//     }
//     return 0;
// }

// int t_mem_test1()
// {
//     int i;
//     int start = 0;
//     uint64 *ptr;
//     memset(mem_test, 0, sizeof(mem_test));
//     int aaa = 0;
//     for (i = 0; i < PAGE_CNT; i++) {
//         if (i % 3 == 0)
//             ptr = (uint64 *)__alloc_pages(0, 3);
//         else if (i % 3 == 1)
//             ptr = (uint64 *)__alloc_pages(0, 1);
//         else if (i % 3 == 2)
//             ptr = (uint64 *)__alloc_page(0);
//         else
//             ptr = NULL;
//         if (!ptr) {
//             panic("__alloc_page returned NULL\n");
//         }
//         // printk("i: %d, ptr: %p\n",i, ptr);
//         if (ele_exist(ptr)) {
//             mm_debug();
//             panic("PAGE_CNT ele_exist\n");
//         }
//         mem_test[start + i] = ptr;

//         if (i % 100 == 0) {
//             printk("100- page: %d\n", aaa++);
//         }
//     }
//     printk("Page allocation successful\n");
//     start += PAGE_CNT;

//     struct kmem_cache *slabs[] = {
//             &task_struct_kmem_cache, &thread_info_kmem_cache, &buf_kmem_cache, &timer_kmem_cache, &efs_inode_kmem_cache, &efs_dentry_kmem_cache, &file_kmem_cache, &tf_kmem_cache, &vma_kmem_cache};
//     int slab_counts[] = {SLAB_TASK_CNT, SLAB_TINFO_CNT, SLAB_BUF_CNT, SLAB_TIMER_CNT, SLAB_INDDE_CNT, SLAB_DENTRY_CNT, SLAB_FILE_CNT, SLAB_TF_CNT, SLAB_VMA_CNT};

//     for (int j = 0; j < sizeof(slab_counts) / sizeof(slab_counts[0]); j++) {
//         for (i = 0; i < slab_counts[j]; i++) {
//             ptr = kmem_cache_alloc(slabs[j]);
//             if (!ptr) {
//                 panic("kmem_cache_alloc returned NULL\n");
//             }
//             // printk("ptr: %p\n", ptr);
//             if (ele_exist(ptr)) {
//                 panic("SLAB %d-%d allocation ele_exist\n", j, i);
//             }
//             mem_test[start + i] = ptr;
//         }
//         printk("SLAB allocation for index %d successful\n", j);
//         start += slab_counts[j];
//     }

//     int kmalloc_sizes[] = {16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192};
//     int kmalloc_counts[] = {KMALLOC16, KMALLOC32, KMALLOC64, KMALLOC128, KMALLOC256, KMALLOC512, KMALLOC1024, KMALLOC2048, KMALLOC4096, KMALLOC8192};

//     for (int j = 0; j < sizeof(kmalloc_sizes) / sizeof(kmalloc_sizes[0]); j++) {
//         for (i = 0; i < kmalloc_counts[j]; i++) {
//             ptr = (uint64 *)kmalloc(kmalloc_sizes[j], 0);
//             if (!ptr) {
//                 panic("kmalloc returned NULL\n");
//             }
//             if (ele_exist(ptr)) {
//                 panic("kmalloc %d-%d ele_exist\n", j, i);
//             }
//             mem_test[start + i] = ptr;
//         }
//         printk("kmalloc allocation for size %d successful\n", kmalloc_sizes[j]);
//         start += kmalloc_counts[j];
//     }

//     return 0;
// }

// extern void mm_debug2();
// int t_mem_test2()
// {
//     mm_debug();
//     mm_debug2();

//     int i;
//     uint64 *ptr;
//     memset(mem_test, 0, sizeof(mem_test));
//     // int aaa = 0;
//     for (i = 0; i < PAGE_CNT; i++) {
//         ptr = (uint64 *)__alloc_pages(0, i % 4);
//         if (!ptr) {
//             panic("__alloc_page returned NULL\n");
//         }
//         // printk("i: %d, ptr: %p\n",i, ptr);
//         if (ele_exist(ptr)) {
//             mm_debug();
//             panic("PAGE_CNT ele_exist\n");
//         }
//         mem_test[i] = ptr;

//         if (i % 100 == 0) {
//             // printk("100- page: %d\n", aaa++);
//         }
//     }

//     for (i = 0; i < PAGE_CNT; i++) {
//         __free_pages(mem_test[i], i % 4);
//         mem_test[i] = NULL;
//     }

//     printk("------------FREE ALL------------\n");

//     mm_debug();
//     mm_debug2();

//     for (i = 0; i < PAGE_CNT; i++) {
//         ptr = (uint64 *)__alloc_pages(0, i % 4);
//         if (!ptr) {
//             panic("__alloc_page returned NULL\n");
//         }
//         // printk("i: %d, ptr: %p\n",i, ptr);
//         if (ele_exist(ptr)) {
//             mm_debug();
//             panic("PAGE_CNT ele_exist\n");
//         }
//         mem_test[i] = ptr;

//         if (i % 100 == 0) {
//             // printk("100- page: %d\n", aaa++);
//         }
//     }

//     printk("Page allocation successful\n");

//     for (i = 0; i < PAGE_CNT; i++) {
//         __free_pages(mem_test[i], i % 4);
//         mem_test[i] = NULL;
//     }

//     printk("------------FREE ALL------------\n");

//     for (i = 0; i < PAGE_CNT; i++) {
//         ptr = (uint64 *)__alloc_pages(0, i % 3);
//         if (!ptr) {
//             panic("__alloc_page returned NULL\n");
//         }
//         // printk("i: %d, ptr: %p\n",i, ptr);
//         if (ele_exist(ptr)) {
//             mm_debug();
//             panic("PAGE_CNT ele_exist\n");
//         }
//         mem_test[i] = ptr;

//         if (i % 100 == 0) {
//             // printk("100- page: %d\n", aaa++);
//         }
//     }

//     printk("Page allocation successful\n");

//     for (i = 0; i < PAGE_CNT; i++) {
//         __free_pages(mem_test[i], i % 3);
//         mem_test[i] = NULL;
//     }

//     printk("------------FREE ALL------------\n");

//     printk("over...\n");
//     mm_debug();
//     mm_debug2();
//     return 0;
// }

// int t_mem_test3()
// {
//     mm_debug();
//     mm_debug2();
//     int i;
//     uint64 *ptr;
//     memset(mem_test, 0, sizeof(mem_test));
//     // int aaa = 0;
//     printk("----------1----------\n");
//     for (i = 0; i < PAGE_CNT; i++) {
//         ptr = (uint64 *)__alloc_pages(0, i % 4);
//         if (!ptr) {
//             panic("__alloc_page returned NULL\n");
//         }
//         // printk("i: %d, ptr: %p\n",i, ptr);
//         if (ele_exist(ptr)) {
//             mm_debug();
//             panic("PAGE_CNT ele_exist\n");
//         }
//         mem_test[i] = ptr;

//         if (i % 100 == 0) {
//             // printk("100- page: %d\n", aaa++);
//         }
//     }

//     mm_debug();
//     mm_debug2();

//     printk("----------2---------- free forhead half\n");
//     for (i = 0; i < PAGE_CNT / 2; i++) {
//         __free_pages(mem_test[i], i % 4);
//         mem_test[i] = NULL;
//     }

//     printk("----------3---------- alloc forhead half\n");
//     for (i = 0; i < PAGE_CNT / 2; i++) {
//         ptr = (uint64 *)__alloc_pages(0, i % 4);
//         if (!ptr) {
//             panic("__alloc_page returned NULL\n");
//         }
//         // printk("i: %d, ptr: %p\n",i, ptr);
//         if (ele_exist(ptr)) {
//             mm_debug();
//             panic("PAGE_CNT ele_exist\n");
//         }
//         mem_test[i] = ptr;

//         if (i % 100 == 0) {
//             // printk("100- page: %d\n", aaa++);
//         }
//     }

//     mm_debug();
//     mm_debug2();

//     printk("----------4---------- free posthead half\n");
//     for (i = PAGE_CNT / 2; i < PAGE_CNT; i++) {
//         __free_pages(mem_test[i], i % 4);
//         mem_test[i] = NULL;
//     }

//     printk("----------5---------- alloc posthead half\n");
//     for (i = PAGE_CNT / 2; i < PAGE_CNT; i++) {
//         ptr = (uint64 *)__alloc_pages(0, i % 4);
//         if (!ptr) {
//             panic("__alloc_page returned NULL\n");
//         }
//         // printk("i: %d, ptr: %p\n",i, ptr);
//         if (ele_exist(ptr)) {
//             mm_debug();
//             panic("PAGE_CNT ele_exist\n");
//         }
//         mem_test[i] = ptr;

//         if (i % 100 == 0) {
//             // printk("100- page: %d\n", aaa++);
//         }
//     }

//     // mm_debug();
//     // mm_debug2();

//     printk("----------6---------- free all\n");
//     for (i = 0; i < PAGE_CNT; i++) {
//         __free_pages(mem_test[i], i % 4);
//         mem_test[i] = NULL;
//     }

//     // printk("----------7----------\n");
//     // for (i = 0; i < PAGE_CNT; i++) {
//     //     __free_pages(mem_test[i], i % 4);
//     //     mem_test[i] = NULL;
//     // }

//     printk("over...\n");
//     mm_debug();
//     mm_debug2();
//     return 0;
// }

// int t_mem_test4()
// {
//     int i;
//     uint64 *ptr;
//     memset(mem_test, 0, sizeof(mem_test));
//     struct kmem_cache *slabs[] = {
//             &task_struct_kmem_cache, &thread_info_kmem_cache, &buf_kmem_cache, &timer_kmem_cache, &efs_inode_kmem_cache, &efs_dentry_kmem_cache, &file_kmem_cache, &tf_kmem_cache, &vma_kmem_cache};
//     int slab_counts[] = {SLAB_TASK_CNT, SLAB_TINFO_CNT, SLAB_BUF_CNT, SLAB_TIMER_CNT, SLAB_INDDE_CNT, SLAB_DENTRY_CNT, SLAB_FILE_CNT, SLAB_TF_CNT, SLAB_VMA_CNT};
//     int slab_len = sizeof(slab_counts) / sizeof(slab_counts[0]);
//     for (i = 0; i < SLAB_ALL; i++) {
//         ptr = kmem_cache_alloc(slabs[i % slab_len]);
//         if (!ptr) {
//             panic("kmem_cache_alloc returned NULL\n");
//         }
//         if (ele_exist(ptr)) {
//             panic("SLAB - %d allocation ele_exist\n", i);
//         }
//         mem_test[i] = ptr;
//     }
//     printk("1\n");
//     for (i = 0; i < SLAB_ALL / 2; i++) {
//         kmem_cache_free(slabs[i % slab_len], mem_test[i]);
//         mem_test[i] = NULL;
//     }
//     printk("2\n");

//     for (i = 0; i < SLAB_ALL / 2; i++) {
//         ptr = kmem_cache_alloc(slabs[i % slab_len]);
//         if (!ptr) {
//             panic("kmem_cache_alloc returned NULL\n");
//         }
//         if (ele_exist(ptr)) {
//             panic("SLAB - %d allocation ele_exist\n", i);
//         }
//         mem_test[i] = ptr;
//     }
//     printk("3\n");

//     for (i = SLAB_ALL / 2; i < SLAB_ALL; i++) {
//         kmem_cache_free(slabs[i % slab_len], mem_test[i]);
//         mem_test[i] = NULL;
//     }
//     printk("4\n");

//     for (i = SLAB_ALL / 2; i < SLAB_ALL; i++) {
//         ptr = kmem_cache_alloc(slabs[i % slab_len]);
//         if (!ptr) {
//             panic("kmem_cache_alloc returned NULL\n");
//         }
//         if (ele_exist(ptr)) {
//             panic("SLAB - %d allocation ele_exist\n", i);
//         }
//         mem_test[i] = ptr;
//     }
//     printk("5\n");
//     return 0;
// }
