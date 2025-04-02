#include "core/proc.h"
#include "core/sched.h"
#include "core/vm.h"
#include "fs/file.h"
#include "lib/fifo.h"
#include "core/locks/semaphore.h"
#include "core/locks/spinlock.h"
#include "mm/kmalloc.h"
#include "mm/mm.h"
#include "riscv.h"

extern int mappages(pagetable_t pagetable, uint64 va, uint64 pa, uint64 size, int perm);

struct
{
    spinlock_t lock;
    struct fifo queue;
    semaphore_t sem;
} kswap;

static __attribute__((noreturn)) void kswapd(void *args)
{
    for (;;) {
        sem_wait(&kswap.sem);

        spin_lock(&kswap.lock);
        struct fault_args_struct *fas = list_entry(fifo_pop(&kswap.queue), struct fault_args_struct, list);
        spin_unlock(&kswap.lock);

        struct mm_struct *mm = fas->mm;
        struct vm_area_struct *v = fas->vma;
        struct thread_info *t = fas->thread;

        file_llseek(v->vm_file, v->vm_pgoff * PGSIZE + PGROUNDDOWN(fas->fault_addr - v->vm_start), SEEK_SET);
        uint64 *new_page = __alloc_page(0);
        file_read(v->vm_file, new_page, PGSIZE);
        // 映射到原来线程的页表去
        mappages(mm->pgd, PGROUNDDOWN(fas->fault_addr), (uint64)new_page, PGSIZE, vma_extra_prot(v) | PTE_U);
#ifdef DEBUG_SF_PFMAP
        printk("pid: %d, f-maps: %p - %p\n", t->pid, PGROUNDDOWN(fas->fault_addr), (uint64)new_page);
#endif

        kfree(fas);
        // 这里已经映射完了，需要唤醒原来的线程继续执行,并释放资源
        wakeup_process(t);
    }
}

// 目前我们只处理文件缺页
void kswap_wake(struct thread_info *t, struct vm_area_struct *v, uint64 fault_addr)
{
    assert(v->vm_file != NULL, "kswap_wake");
    struct fault_args_struct *fas = kmalloc(sizeof(struct fault_args_struct), 0);
    fas->thread = t;
    fas->mm = &t->task->mm;
    fas->vma = v;
    fas->fault_addr = fault_addr;
    INIT_LIST_HEAD(&fas->list);

    spin_lock(&kswap.lock);
    fifo_push(&kswap.queue, &fas->list);
    spin_unlock(&kswap.lock);

    sem_signal(&kswap.sem);
}

void kswapd_init()
{
    spin_init(&kswap.lock, "kswap-lock");
    fifo_init(&kswap.queue);
    sem_init(&kswap.sem, 0, "kswap-sem");

    kthread_create(get_init(),kswapd, NULL, "kswapd", NO_CPU_AFF);
}
