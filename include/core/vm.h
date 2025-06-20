#ifndef __VM_H__
#define __VM_H__
#include "std/stddef.h"
#include "core/locks/spinlock.h"
#include "riscv.h"
#include "lib/list.h"

struct vm_operations_struct;
struct thread_info;


// Virtual Memory Area
struct vm_area_struct
{
    // 这里对引用的操作时机：进程创建、进程销毁、kwback。不会出现释放一般时突然有人加引用
    // atomic_t ref;   

    uint64_t vm_start;      // 区域起始地址
    uint64_t vm_end;        // 区域结束地址
    flags64_t vm_prot;    // 区域标志 RWX
    flags_t vm_flags;     // 暂时没有使用
    uint32_t vm_pgoff;      // 文件页偏移
    struct file *vm_file; // 关联文件
    struct vm_area_struct *vm_next;
    struct vm_operations_struct *vm_ops;
};

// https://don7hao.github.io/2015/01/28/kernel/mm_struct/
struct mm_struct
{
    pagetable_t pgd;

    uint64_t start_code;
    uint64_t end_code;

    uint64_t start_data;
    uint64_t end_data;

    uint64_t start_brk;
    uint64_t end_brk;

    uint64_t start_stack;

    uint64_t next_map;    // 我们目前采取最简单的挨个线性映射（不过当文件数映射过多时候会爆）

    struct vm_area_struct *mmap;
    spinlock_t lock; // 保护并发访问
    int map_count;   // VMA 数量

    uint64_t size; // 总内存使用量
};

struct fault_args_struct
{
    struct thread_info *thread;
    struct mm_struct *mm;
    struct vm_area_struct *vma;
    uint64_t fault_addr;
    struct list_head list;
};

struct vm_operations_struct
{
    void (*fault)(struct thread_info *t, struct vm_area_struct *v, uint64_t fault_addr); // 缺页中断
    void (*close)(struct mm_struct *mm, struct vm_area_struct *v);
    void (*dup)(struct mm_struct *mm, struct vm_area_struct *v, struct mm_struct *new_mm);
};

extern struct vm_operations_struct vma_file_ops;
extern struct vm_operations_struct vma_stack_ops;
extern struct vm_operations_struct vma_args_ops;
extern struct vm_operations_struct vma_gen_ops;

extern int vma_is_r(struct vm_area_struct *vma);
extern int vma_is_w(struct vm_area_struct *vma);
extern int vma_is_x(struct vm_area_struct *vma);
extern int vma_extra_prot(struct vm_area_struct *vma);
extern void vma_prot_copy(struct vm_area_struct *vma, pte_t *pte);
extern void vma_insert(struct mm_struct *mm, struct vm_area_struct *vma);
extern struct vm_area_struct *vma_alloc_proghdr(uint64_t start, uint64_t end, uint64_t proghdr_flags, uint32_t pgoff,
                                                struct file *file, struct vm_operations_struct *ops);
extern void vma_list_cat(struct vm_area_struct *vma);


extern struct vm_area_struct *vma_dup(struct vm_area_struct *old_vma);
extern void free_vma(struct vm_area_struct *vma);

extern void kvm_init();
extern void kvm_init_hart();

extern int alloc_user_pgd(struct mm_struct *mm);
extern int alloc_kern_pgd(struct mm_struct *mm);
extern void free_user_pgd(struct mm_struct *mm);

extern void free_user_memory(struct mm_struct *mm);

extern void set_cow_pte(pte_t *pte);

#endif
