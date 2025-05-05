// #include "core/locks/spinlock.h"
// #include "core/proc.h"
// #include "core/timer.h"
// #include "core/vm.h"
// #include "lib/list.h"
// #include "mm/kmalloc.h"
// #include "mm/page.h"
// #include "fs/file.h"

// extern pte_t *walk(pagetable_t pagetable, uint64_t va, int alloc);


// // TODO 放在 vma 根据每个 vma 来刷写?
// // 回写的参数，根据 v + va 确定磁盘位置
// // 需要保存时机的 pa，防止进程挂掉导致内容错误
// struct wback_args {
//     struct vm_area_struct *v;
//     uint64_t va;
//     struct page *page;
//     struct list_head list;
// };

// struct
// {
//     spinlock_t lock;
//     struct list_head wpages;
// } kwback;



// static __attribute__((noreturn)) void kwbackd(void *)
// {
//     struct thread_info *p = myproc();
//     while (1) {
//         thread_timer_sleep(p, 10000);
//         spin_lock(&kwback.lock);
//         if (list_empty(&kwback.wpages)) {
//             spin_unlock(&kwback.lock);
//             continue;
//         }
//         struct wback_args *wa = list_entry(list_pop(&kwback.wpages), struct wback_args, list);
//         struct vm_area_struct *v = wa->v;
//         struct page *page = wa->page;
//         uint64_t va = wa->va;

//         SET_FLAG(&wa->page->flags, PG_WBACKING);  // 设置页面正在被写
//         file_llseek(v->vm_file, v->vm_pgoff * PGSIZE + PGROUNDDOWN(va - v->vm_start), SEEK_SET);
//         spin_unlock(&kwback.lock);
//         file_write(v->vm_file, (void*)PG2PA(page), PGSIZE);

//         kfree(wa);
//     }
// }

// static struct wback_args * wback_args_init( struct vm_area_struct *v ,uint64_t va, struct page*pg)
// {
//     struct wback_args * wa = kmalloc(sizeof(struct wback_args),0);
//     if(!wa){
//         panic("wback_args_init");
//     }
//     wa->v = v;
//     wa->va = va;
//     wa->page = pg;
//     INIT_LIST_HEAD(&wa->list);
//     return wa;
// }

// // 负责处理可写页面的返回
// // 调用此函数将页面加入 kwback,此时可以确保页面被使用且不再伙伴系统中
// // 我们复用其 buddy 链表，以节省内存开支
// void kwback_page_wake(struct mm_struct *mm,  struct vm_area_struct *v ,uint64_t va)
// {
//     if ((va % PGSIZE) != 0)
//         panic("kwback_page_wake (va % PGSIZE) != 0\n");

//     pte_t *pte = walk(mm->pgd, va, 0);
//     if (!pte || !(*pte & PTE_V))
//         panic("kwback_page_wake\n");

//     struct page *pg = PA2PG(PTE2PA(*pte));
//     spin_lock(&pg->lock);
//     get_page(pg);  // 确保页面的生存周期

//     atomic_inc(&v->ref);
//     struct wback_args * wa = wback_args_init(v,va,pg);
//     spin_unlock(&pg->lock);

//     spin_lock(&kwback.lock);
//     list_add_head(&wa->list, &kwback.wpages);
//     spin_unlock(&kwback.lock);
// }

// // 我们对每个页面采取自旋锁 + 标志位，每个用户程序使用 while test_flag sched的方式
// // 使得产生类似睡眠效果。来代替对每个页面新增一个睡眠锁结构
// // 返回值 !=0 则说明
// flags_t kwback_page_test(struct mm_struct *mm, uint64_t va)
// {
//     if ((va % PGSIZE) != 0)
//         panic("kwback_page_test (va % PGSIZE) != 0\n");

//     pte_t *pte = walk(mm->pgd, va, 0);
//     if (!pte || !(*pte & PTE_V))
//         panic("kwback_page_test\n");

//     struct page *pg = PA2PG(PTE2PA(*pte));

//     spin_lock(&pg->lock);
//     flags_t flag = TEST_FLAG(&pg->flags, PG_WBACKING);
//     spin_unlock(&pg->lock);
//     return flag;
// }

// void kwback_init()
// {
//     spin_init(&kwback.lock, "kwback");
//     kwback.pages = 0;
//     INIT_LIST_HEAD(&kwback.wpages);
// }
