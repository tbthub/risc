#include "core/proc.h"
#include "core/vm.h"
#include "elf.h"
#include "lib/string.h"
#include "mm/mm.h"
#include "mm/page.h"
#include "mm/slab.h"
#include "riscv.h"
#include "sys.h"

extern pte_t *walk(pagetable_t pagetable, uint64 va, int alloc);
extern int mappages(pagetable_t pagetable, uint64 va, uint64 pa, uint64 size, int perm);
extern void kswap_wake(struct thread_info *t, struct vm_area_struct *v, uint64 fault_addr);
extern void __attribute__((unused)) vm2pa_show(struct mm_struct *mm);

// debug
void vma_list_cat(struct vm_area_struct *vma)
{
    while (vma) {
        printk("start - end %p - %p\n", vma->vm_start, vma->vm_end);
        vma = vma->vm_next;
    }
}

inline int vma_is_r(struct vm_area_struct *vma)
{
    return (vma->vm_prot & VM_PROT_READ);
}

inline int vma_is_w(struct vm_area_struct *vma)
{
    return (vma->vm_prot & VM_PROT_WRITE);
}

inline int vma_is_x(struct vm_area_struct *vma)
{
    return (vma->vm_prot & VM_PROT_READ);
}

int vma_extra_prot(struct vm_area_struct *vma)
{
    int vma_perm = vma->vm_prot & 0b1110;
    return vma_perm;
}

struct vm_area_struct *vma_dup(struct vm_area_struct *old_vma)
{
    struct vm_area_struct *v = kmem_cache_alloc(&vma_kmem_cache);
    *v = *old_vma;
    v->vm_next = NULL;
    return v;
}

void free_vma(struct vm_area_struct *vma)
{
    kmem_cache_free(&vma_kmem_cache, vma);
}

void vma_prot_copy(struct vm_area_struct *vma, pte_t *pte)
{
    uint64 vma_perm = vma->vm_prot & 0b1110;  // 取出bit1,2,3（R/W/X）
    uint64 pte_val = *pte;

    pte_val &= ~0b1110;
    pte_val |= vma_perm;

    // 更新PTE
    *pte = pte_val;
}

// 对于所有的 vma 来说，我们默认为 start < end，并以此来寻找地址对应的 vma
// addr >= v->vm_start && addr <= v->vm_end
// 但是堆栈是个例外，堆栈是往下增长的，因此堆栈的 end 是栈顶，也就是 USER_STACK_TOP
// 堆栈每一次因为页面不足发生缺页，要往下移动 start，但是有个最大的限度。
// 并不会一次性把堆栈所有空间 vma 分配完成
struct vm_area_struct *find_vma(struct mm_struct *mm, uint64 addr)
{
    // TODO 我们暂时使用比较简单的线性查找，以后有机会改为红黑树
    struct vm_area_struct *v;
    spin_lock(&mm->lock);
    v = mm->mmap;
    while (v) {
        if (addr >= v->vm_start && addr <= v->vm_end)
            break;
        v = v->vm_next;
    }
    spin_unlock(&mm->lock);
    return v;
}

void vma_insert(struct mm_struct *mm, struct vm_area_struct *vma)
{
    if (mm->mmap)
        vma->vm_next = mm->mmap;
    mm->mmap = vma;
}

struct vm_area_struct *vma_alloc_proghdr(uint64 start, uint64 end, uint64 proghdr_flags, uint32 pgoff, void* file, struct vm_operations_struct *ops)
{
    struct vm_area_struct *v = kmem_cache_alloc(&vma_kmem_cache);
    assert(v != NULL, "vma_alloc_proghdr");
    if (proghdr_flags & ELF_PROG_FLAG_EXEC)
        SET_FLAG(&v->vm_prot, VM_PROT_EXEC);
    if (proghdr_flags & ELF_PROG_FLAG_WRITE)
        SET_FLAG(&v->vm_prot, VM_PROT_WRITE);
    if (proghdr_flags & ELF_PROG_FLAG_READ)
        SET_FLAG(&v->vm_prot, VM_PROT_READ);
    v->vm_start = start;
    v->vm_end = end;
    v->vm_pgoff = pgoff;
    v->vm_file = file;
    v->vm_ops = ops;
    v->vm_next = NULL;
    v->vm_flags = 0;
    return v;
}

// size 要求页对齐,
// 返回 end
static uint64 find_free_region_map(struct mm_struct *mm, int aligned_size)
{
    uint64 top = mm->next_map;
    mm->next_map = mm->next_map - aligned_size - PGSIZE;

    if (top < USER_MAP_BOTTOM || mm->next_map < USER_MAP_BOTTOM) {
        return 0;  // 没有足够的空间
    }

    return top;
}

static void vma_gen_close(struct mm_struct *mm, struct vm_area_struct *v)
{
    assert(v != NULL, "vma_gen_close v == NULL");
    pte_t *pte;
    pte = walk(mm->pgd, v->vm_start, 0);
    for (uint64 _i_ = 0, _va_ = v->vm_start; _va_ < v->vm_end; _va_ += PGSIZE, _i_++) {
        if (!pte) {
            pte = walk(mm->pgd, _va_ + PGSIZE, 0);
            continue;
        }
        else if (*pte & PTE_V) {
            __free_page((void *)PTE2PA(*pte));
            *pte = 0;
        }

        pte += 1;
        if (((uint64)pte & (PGSIZE - 1)) == 0)
            pte = walk(mm->pgd, _va_ + PGSIZE, 0);
    }
}

static void vma_gen_dup(struct mm_struct *mm, struct vm_area_struct *v, struct mm_struct *new_mm)
{
    assert(v != NULL, "vma_gen_dup v == NULL");
    pte_t *pte;
    pte_t *pte2;
    struct vm_area_struct *vma2;

    pte = walk(mm->pgd, v->vm_start, 0);
    for (uint64 _i_ = 0, va = v->vm_start; va < v->vm_end; va += PGSIZE, _i_++) {
        if (!pte) {
            pte = walk(mm->pgd, va + PGSIZE, 0);
            continue;
        }
        else if (*pte & PTE_V) {
            if (*pte & PTE_W)
                set_cow_pte(pte);
            get_page(PA2PG(PTE2PA(*pte)));

            pte2 = walk(new_mm->pgd, va, 1);
            *pte2 = *pte;
        }

        pte += 1;
        if (((uint64)pte & (PGSIZE - 1)) == 0)
            pte = walk(mm->pgd, va + PGSIZE, 0);
    }
    vma2 = vma_dup(v);
    vma_insert(new_mm, vma2);
}

static void vma_gen_file_fault(struct thread_info *t, struct vm_area_struct *v, uint64 fault_addr)
{
    assert(v->vm_file != NULL, "vma_gen_file_fault");

    // * 注意，必须先获取到进程的锁之后才能去 kswap_wake
    // ＊ 你也不想我这边还没进入调度那边就已经完成了吧，为了避免不必要的麻烦
    spin_lock(&t->lock);
    t->state = SLEEPING;

    kswap_wake(t, v, fault_addr);

    sched();  // 会去运行其他的线程
    // 定时器处理结束后唤醒，让其上调度队列，然后经过调度回到这里
}

static void vma_gen_file_dup(struct mm_struct *mm, struct vm_area_struct *v, struct mm_struct *new_mm)
{
    assert(v->vm_file != NULL, "vma_gen_file_dup");
    vma_gen_dup(mm, v, new_mm);
    new_mm->mmap->vm_file = k_file_mmap_dup(v->vm_file);
    assert(v->vm_file != NULL, "v->vm_file != NULL vma_gen_file_dup");
}

static void vma_gen_file_close(struct mm_struct *mm, struct vm_area_struct *v)
{
    assert(v->vm_file != NULL, "vma_gen_file_close");
    vma_gen_close(mm, v);
    k_file_mmap_close(v->vm_file);
}

// 堆栈的缺页发生在越界的时候
static void vma_gen_stack_fault(struct thread_info *t, struct vm_area_struct *v, uint64 fault_addr)
{
    if (v->vm_end - v->vm_start >= USER_STACK_SIZE)
        panic("stack overhidden\n");  // 杀死

    uint64 *new_page = __alloc_page(0);
    if (!new_page)
        panic("vma_gen_stack_fault\n");
    // printk("vma_gen_stack_fault %s, addr: %p\n", t->name,fault_addr);
    mappages(t->task->mm.pgd, PGROUNDDOWN(fault_addr), (uint64)new_page, PGSIZE, vma_extra_prot(v) | PTE_U);
#ifdef DEBUG_SF_PFMAP
    printk("pid: %d, s-maps: %p - %p\n", t->pid, PGROUNDDOWN(fault_addr), (uint64)new_page);
#endif
    v->vm_start -= PGSIZE;
}

// * 参数页面理论上不会发生缺页
static void vma_gen_args_fault(struct thread_info *t, struct vm_area_struct *v, uint64 fault_addr)
{
    panic("vma_gen_args_fault\n");
}

struct vm_operations_struct vma_file_ops = {
        .close = vma_gen_file_close,
        .dup = vma_gen_file_dup,
        .fault = vma_gen_file_fault,
};

struct vm_operations_struct vma_stack_ops = {
        .close = vma_gen_close,
        .dup = vma_gen_dup,
        .fault = vma_gen_stack_fault,
};

struct vm_operations_struct vma_args_ops = {
        .close = vma_gen_close,
        .dup = vma_gen_dup,
        .fault = vma_gen_args_fault,
};

// addr：
// 用户期望的起始虚拟地址。
// 一般情况下，用户会指定 NULL，由内核决定合适的起始地址。
int64 do_mmap(void *addr, uint32 len, flags64_t prot, flags_t flags, fd_t fd, uint32 offset)
{
    uint64 aligned_len;
    struct mm_struct *mm = &myproc()->task->mm;
    struct vm_area_struct *v;

    void *file_ctx = k_file_mmap_init(fd);

    if (file_ctx == NULL){
        return -1;
    }

    if (len <= 0 || offset & (PGSIZE - 1)) {
        printk("do_mmap len <= 0 || offset & (PGSIZE - 1)\n");
        return -1;
    }

    aligned_len = PGROUNDUP(len);

    v = kmem_cache_alloc(&vma_kmem_cache);
    assert(v != NULL, "do_mmap");

    v->vm_file = file_ctx;

    if ((void *)addr == NULL)
        v->vm_end = find_free_region_map(mm, aligned_len);
    else
        v->vm_end = (uint64)addr + aligned_len;
    v->vm_start = v->vm_end - aligned_len + 1;
    v->vm_prot = prot;
    v->vm_ops = &vma_file_ops;
    SET_FLAG(&v->vm_flags, flags);

    vma_insert(mm, v);

    return (int64)v->vm_start;
}

int64 do_munmap(void *addr, uint32 len)
{
    // struct vm_area_struct *v;
    // struct mm_struct *mm = &myproc()->task->mm;
    // spin_lock(&mm->lock);
    // v = find_vma(mm, (uint64)addr);
    // if (!v)
    //     panic("do_munmap, v is NULL\n");
    return 0;
}
