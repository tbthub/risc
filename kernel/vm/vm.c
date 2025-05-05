#include "core/vm.h"

#include "core/locks/spinlock.h"
#include "core/proc.h"
#include "core/trap.h"
#include "defs.h"
#include "elf.h"
#include "fs/file.h"
#include "lib/atomic.h"
#include "lib/string.h"
#include "mm/mm.h"
#include "mm/page.h"
#include "riscv.h"
#include "std/stddef.h"
#include "std/stdio.h"

// 内核页表
pagetable_t kernel_pagetable;
extern struct vm_area_struct *find_vma(struct mm_struct *mm, uint64_t addr);
extern void kswap_wake(struct thread_info *t, struct vm_area_struct *v, uint64_t fault_addr);

//   64 位的虚拟地址字段分布
//   39..63 -- must be zero.
//   30..38 -- 9 bits of level-2 index.
//   21..29 -- 9 bits of level-1 index.
//   12..20 -- 9 bits of level-0 index.
//    0..11 -- 12 bits of byte offset within the page.

// SV39 格式下的页表项分布
//  54..63  -- reserved
//  28..53  -- PPN[2]
//  19..27  -- PPN[1]
//  10..18  -- PPN[0]
//  9..8    -- RSW  (Reserved for Software)
//  保留,操作系统可用，比如用这个来区分普通的不可写、写时复制 7       -- D
//  (Dirty)
//  处理器记录自从页表项上的这一位被清零之后，页表项的对应虚拟页面是否被修改过。
//  6       -- A    (Accessed)
//  处理器记录自从页表项上的这一位被清零之后，页表项的对应虚拟页面是否被访问过
//  5       -- G    (Global)    指示该页表项指向的物理页面在所有进程中共享
//  4       -- U    (User)      控制索引到这个页表项的对应虚拟页面是否在 CPU
//  处于 U 特权级的情况下是否被允许访问； 3       -- X 2       -- W 1       -- R
//  0       -- V    (Valid)     仅当位 V 为 1 时，页表项才是有效的；

// 返回最下面的那个页表项的地址,并根据 alloc 的值看是否决定分配
pte_t *walk(pagetable_t pagetable, uint64_t va, int alloc)
{
    if (va >= MAXVA) {
        printk("vm.c walk: Virtual address out of bounds!");
        // (杀死当前进程?)
        return NULL;
    }

    int level;

    for (level = 2; level > 0; level--) {
        // 提取出虚拟地址的level级别的每9位，根据这个9位在当前页表(这个pagetable也在反复更新)中查找
        pte_t *pte = &pagetable[PX(level, va)];

        // 如果该页表项有效（Valid 位）
        // 额外地，V 位有效则一定是有映射实际的物理页面的，
        // 只不过该实际页面的内容在不在内存中就是 swap 的事情了
        if (*pte & PTE_V) {
            pagetable = (pagetable_t)PTE2PA(*pte);
        }

        // 否则无效，这里看需不需要分配，即alloc的值
        // 由于我们采用树形结构，因此不用的 pte
        // （没有实际映射的）可以不分配，可以节省一大块空间
        // 如果直接平铺开，则每个 pte 都要提前分配好

        else {
            // 如果不需要分配、或者已经没有页面可以分配了
            if (!alloc || (pagetable = (pagetable_t)__alloc_page(0)) == NULL)
                return NULL;
            // 需要分配的话上面 if 判断中已申请到一个页面
            // 注：申请到的页面地址是真实的物理地址
            memset(pagetable, 0, PGSIZE);
            // 设置页表项的属性
            *pte = PA2PTE(pagetable) | PTE_V;
        }
    }
    // 返回根据最后9位找到的 pte 地址
    return &pagetable[PX(0, va)];
}

// 为从 va 开始的虚拟地址创建 PTE（页表项），这些虚拟地址对应于从 pa
// 开始的物理地址。 va 和 size 必须是页对齐的（size为页面的整数倍）。 成功时返回
// 0，若 walk() 无法分配所需的页表页，则返回 -1。 从 va 到 va + sz
// 的虚拟地址范围映射到从 pa 到 pa + sz 的物理地址范围。 再次强调下，va
// 区域连续，pa 区域也连续的！！！。
int mappages(pagetable_t pagetable, uint64_t va, uint64_t pa, uint64_t size, int perm)
{
    uint64_t a, last;
    pte_t *pte;
    if ((va % PGSIZE) != 0)
        panic("mappages: va: %p not aligned\n",va);

    if ((size % PGSIZE) != 0)
        panic("mappages: size not aligned");

    if (size == 0)
        panic("mappages: size");

    a = va;
    // 指向最后一个映射的页面，由于个数原因，最后要减去一个页面大小。
    // 比如：0x8000_0000开始的这个页面 sz = 0x1000,则最后映射的页面就是
    // 0x8000_0000
    last = va + size - PGSIZE;

    spin_lock(&mem_map.lock);
    for (;;) {
        if ((pte = walk(pagetable, a, 1)) == NULL)
            return ERR;

        if (*pte & PTE_V)
            panic("vm.c mappages: remap va: %p\n", va);

        // 查看该文件上面对SV39的字段解释
        // 添加页面映射和权限信息
        *pte = PA2PTE(pa) | perm | PTE_V;

        if (a == last)
            break;
        a += PGSIZE;
        pa += PGSIZE;
    }
    spin_unlock(&mem_map.lock);
    return 0;
}

/*
 * 内核页表映射
 */

static void kvm_map(pagetable_t kpgtbl, uint64_t va, uint64_t pa, uint64_t sz, int perm)
{
    if (mappages(kpgtbl, va, pa, sz, perm) != 0)
        panic("vm.c kvm_map: Error!");
}

// 内核虚存初始化
void kvm_init()
{
    pagetable_t kpgtbl;

    kpgtbl = __alloc_page(0);
    if (!kpgtbl)
        panic("Failed to apply for kernel page table space!\n");

    memset(kpgtbl, 0, PGSIZE);

    // 下面开始映射、大部分都是恒等映射

    // uart registers
    kvm_map(kpgtbl, UART0, UART0, PGSIZE, PTE_R | PTE_W);

    // virtio mmio disk interface
    kvm_map(kpgtbl, VIRTIO0, VIRTIO0, PGSIZE, PTE_R | PTE_W);

    // PLIC
    kvm_map(kpgtbl, PLIC, PLIC, 0x4000000, PTE_R | PTE_W);

    // 内核代码段的映射（可读可执行）
    // etext
    // 表示内核代码的结束位置。链接器在内核编译时会生成这个符号，表示内核代码段的末尾地址。参见kernel.ld
    kvm_map(kpgtbl, KERNBASE, KERNBASE, (uint64_t)etext - KERNBASE, PTE_R | PTE_X);

    // 内核数据段的映射
    // kvm_map(kpgtbl, (uint64_t)etext, (uint64_t)etext, PGROUNDUP((uint64_t)end)
    // - (uint64_t)etext, PTE_R | PTE_W);

    // 内核数据段和物理内存的映射（可读可写，包括堆、栈、BSS）
    kvm_map(kpgtbl, (uint64_t)etext, (uint64_t)etext, PHYSTOP - (uint64_t)etext, PTE_R | PTE_W);

    // 下面是我们暂时还没有实现的

    printk("kvm init ok!\n");
    kernel_pagetable = kpgtbl;
}

// 将硬件页表寄存器 satp 切换到内核的页表（stap寄存器保存页表地址），
// 并启用分页功能。
void kvm_init_hart()
{
    w_sstatus(SSTATUS_SUM);
    // 等待之前对页表内存的任何写操作完成。
    sfence_vma();

    // 将页表切换到内核的页表。
    // （此后就开始虚拟地址了,在kvm中，内核页表是恒等映射，内核的虚拟地址与物理地址一样，所以代码继续往下执行，没问题）
    w_satp(MAKE_SATP(kernel_pagetable));

    // 刷新TLB（翻译后备缓冲区）中的过期条目。
    sfence_vma();
}

/*
 * 写时复制、缺页中断
 */

// debug，打印已经映射的页表
void __attribute__((unused)) vm2pa_show(struct mm_struct *mm)
{
    struct vm_area_struct *v = mm->mmap;
    pte_t *pte;
    while (v) {
        spin_lock(&mem_map.lock);
        for (uint64_t va = v->vm_start; va < v->vm_end; va += PGSIZE) {
            pte = walk(mm->pgd, va, 0);
            if (!pte)
                continue;
            if ((*pte & PTE_V) == 0)
                continue;
            printk("va: %p, pa: %p  %d\n", va, PTE2PA(*pte), *pte & 0b1110);
        }
        v = v->vm_next;
        spin_unlock(&mem_map.lock);
    }
}

void set_cow_pte(pte_t *pte)
{
    *pte &= ~PTE_W;   // 清除写权限，设置为只读
    *pte |= PTE_COW;  // 设置为 COW 页面
}

void clear_cow_pte(pte_t *pte)
{
    *pte |= PTE_W;
    *pte &= ~PTE_COW;
}

int is_cow_pte(pte_t *pte)
{
    return (*pte & PTE_COW);
}

// 设置页面正在处理缺页(需互斥情况下)
static inline void set_pf_pte(pte_t *pte)
{
    *pte |= PTE_PF;
}

static inline int is_pf_pte(pte_t *pte)
{
    return (*pte & PTE_PF);
}

static inline void clear_pf_pte(pte_t *pte)
{
    *pte &= ~PTE_PF;
}

static void handle_cow_page_fault(pte_t *pte, uint64_t fault_addr)
{
#ifdef DEBUG_COW
    struct thread_info *th = myproc();
    printk("pid %d: cow page: %p\n", th->pid, PGROUNDDOWN(fault_addr));
#endif
    struct page *pg = PA2PG(PTE2PA(*pte));
    // 这里查看引用数是原子的，但是我们额外加锁保证复制页面数据完全一致
    // 避免一个进程复制给过程中，另外一个进程快速退出后修改页面数据
    spin_lock(&pg->lock);
    if (put_page_test_1(pg)) {
        void *new_page = __alloc_page(0);
        memcpy(new_page, (void *)PGROUNDDOWN(fault_addr), PGSIZE);
#ifdef DEBUG_COW
        printk("pid %d: cow NEW page, cow-maps: %p - %p\n", th->pid, PGROUNDDOWN(fault_addr), (uint64_t)new_page);
#endif
        // 先保留原PTE的权限位(0 - 9,即低10位)
        uint64_t old_flags = *pte & 0x3FF;
        *pte = PA2PTE(new_page) | old_flags;
    }
    else {
        get_page(pg);
#ifdef DEBUG_COW
        printk("pid %d: cow OLD page. addr is %p\n", th->pid, PGROUNDDOWN(fault_addr));
#endif
    }
    spin_unlock(&pg->lock);
    clear_cow_pte(pte);
}

void page_fault_handler(uint64_t fault_addr, uint64_t scause)
{
    assert(intr_get() == 0, "page_fault_handler intr on!\n");
    struct thread_info *th = myproc();

    struct mm_struct *mm = &th->task->mm;
    pte_t *pte;
    struct vm_area_struct *v;

#ifdef DEBUG_PF
    printk("now satp: %p, proc satp: %p\n", r_satp(), MAKE_SATP(th->task->mm.pgd));
    printk("page_fault: pid: %d, name:%s\n", th->pid, th->name);
    printk("scause: %p ", scause);
    printk("spec: %p ", r_sepc());
    printk("stval: %p \n", r_stval());
    printk("mmap: %p \n", th->task->mm.mmap);
    vma_list_cat(th->task->mm.mmap);
#endif
    v = find_vma(mm, fault_addr);
    if (!v) {
        panic("page_fault_handler: illegal addr %p, scause: %p\n", fault_addr, scause);  // TODO 杀死进程，不过我们暂时先报错
        // return;
    }

    switch (scause) {
    case E_INS_PF:
        if (!vma_is_x(v))
            panic("E_INS_PF\n");
        break;

    case E_LOAD_PF:
        if (!vma_is_r(v))
            panic("E_LOAD_PF");
        break;

    case E_STORE_AMO_PF:
        pte = walk(mm->pgd, fault_addr, 0);
        if (pte && (*pte & PTE_V)) {
            if (is_cow_pte(pte)) {
                handle_cow_page_fault(pte, fault_addr);
                return;
            }
            // 如果不是写时复制，那么权限错误，应该直接杀死
            else if (!vma_is_w(v))
                panic("E_STORE_AMO_PF");
        }
        break;

    default:
        break;
    }

    spin_lock(&mm->lock);
    pte = walk(mm->pgd, fault_addr, 1);

    // 无效、未在处理->（第一个线程）
    if (!(*pte & PTE_V) || !is_pf_pte(pte)) {
        set_pf_pte(pte);  // 标记这个页面正在被处理
        spin_unlock(&mm->lock);
        v->vm_ops->fault(th, v, fault_addr);

        // 缺页处理完成，清除 PTE_PF 标志
        spin_lock(&mm->lock);
        clear_pf_pte(pte);
        spin_unlock(&mm->lock);
    }
    else {
        // 页面正在被其他线程缺页处理中
        while (is_pf_pte(pte)) {
            spin_unlock(&mm->lock);
            yield();  // (就绪态)让出 CPU，等待缺页处理完成
            spin_lock(&mm->lock);
        }
        spin_unlock(&mm->lock);
    }

    // 如果发生进程切换（内核中断没有改变页表的）
    // 且当前内核的内核页表并非该进程的内核页表，则需要修改
    // if (r_satp() != MAKE_SATP(mm->pgd)) {
    //     sfence_vma();
    //     w_satp(MAKE_SATP(mm->pgd));
    //     sfence_vma();
    // }
    sfence_vma();
    w_satp(MAKE_SATP(mm->pgd));
    sfence_vma();

    // vm2pa_show(&th->task->mm);
    // printk("----------------\n");
}

/*
 * 用户虚存管理(部分)
 */

int alloc_user_stack(struct mm_struct *mm, tid_t tid)
{
    // 理论上。。。栈第一个页面也可以通过缺页中断实现，不过我们为了效率，预分配栈
    uint64_t *stack = NULL;
    struct vm_area_struct *v = NULL;

    stack = __alloc_page(0);
    if (!stack)
        return -1;

    v = vma_alloc_proghdr(PGROUNDDOWN(USER_STACK_TOP(tid)), PGROUNDUP(USER_STACK_TOP(tid) + 1) - 1,
     ELF_PROG_FLAG_READ | ELF_PROG_FLAG_WRITE, 0, NULL, &vma_stack_ops);

    vma_insert(mm, v);
    mappages(mm->pgd, PGROUNDDOWN(USER_STACK_TOP(tid)), (uint64_t)stack, PGSIZE, PTE_R | PTE_W | PTE_U);
    return 0;
}

int alloc_user_pgd(struct mm_struct *mm)
{
    assert(mm != NULL, "alloc_user_pgd\n");
    mm->pgd = __alloc_page(0);
    if (!mm->pgd)
        return -1;
    // printk("alloc_user_pgd, pgd: %p, kern:%p\n", mm->pgd, kernel_pagetable);
    // 把内核也映射到用户地址空间
    memcpy(mm->pgd, kernel_pagetable, PGSIZE);
    return 0;
}

int alloc_kern_pgd(struct mm_struct *mm)
{
    assert(mm != NULL, "alloc_user_pgd\n");
    mm->pgd = __alloc_page(0);
    if (!mm->pgd) {
        panic("alloc_kern_pgd mm->pgd\n");
        return -1;
    }

    memcpy(mm->pgd, kernel_pagetable, PGSIZE);
    // mm->pgd = kernel_pagetable;
    return 0;
}

void free_user_pgd(struct mm_struct *mm)
{
    if (!mm) {
        panic("mm\n");
    }

    if (mm->pgd) {
        // printk("free_user_pgd, pgd: %p ref:%d \n", mm->pgd,page_count(PA2PG(mm->pgd)));
        __free_page(mm->pgd);
        mm->pgd = NULL;
    }
}

// ! 我们暂时没有实现页表本身的释放，后面补充
// ! 页表自身需要父进程来回收，但是我们在这里有 bug
void free_user_memory(struct mm_struct *mm)
{
    // 根据 mm->map 指导释放内存，尽管页表可能没有映射
    struct vm_area_struct *vma = mm->mmap;
    struct vm_area_struct *tmp = NULL;
    while (vma) {
        vma->vm_ops->close(mm, vma);
        tmp = vma->vm_next;
        free_vma(vma);
        vma = tmp;
    }
    mm->mmap = NULL;
}

// 用户程序引导代码，所有程序共享
void user_init()
{
    pte_t *pte = walk(kernel_pagetable, (uint64_t)user_entry, 0);
    assert(pte != NULL, "user_init\n");
    *pte |= (PTE_R | PTE_X | PTE_U);
}
