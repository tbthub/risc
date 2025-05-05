#include "kprobe.h"

#include "core/export.h"
#include "core/locks/spinlock.h"
#include "core/module.h"
#include "std/string.h"
#include "mm/kmalloc.h"
#include "mm/mm.h"
#include "riscv.h"
#include "std/stddef.h"
#define EXEC_TYPE 1
#define ATTCH_TYPE 0

extern void kprobe_start();
extern void kprobe_end();

extern void kprobe_exec_entry();
extern void kprobe_exec_entry_set();

extern void kprobe_attch_entry();
extern void kprobe_attch_entry_set();

extern pagetable_t kernel_pagetable;
extern pte_t *walk(pagetable_t pagetable, uint64_t va, int alloc);

// ! 注意：使用 exec 附加的函数需要打开对应的编译选项
static struct kprobe *alloc_kprobe()
{
    struct kprobe *kp = kmalloc(sizeof(struct kprobe), 0);
    if (!kp)
        return NULL;
    kp->page = __alloc_page(0);
    if (!kp->page) {
        kfree(kp);
        return NULL;
    }
    return kp;
}

static void free_kprobe(struct kprobe *kp)
{
    if (!kp)
        return;
    if (kp->page) {
        __free_page(kp->page);
    }
    kfree(kp);
}

static void set_pte_writable(pte_t *pte)
{
    push_off();
    *pte |= PTE_W;
    sfence_vma();
    asm volatile("fence.i");
}

static void reset_pte_writable(pte_t *pte)
{
    pop_off();
    *pte &= ~PTE_W;
    sfence_vma();
    asm volatile("fence.i");
}

static void set_pte_execable(pte_t *pte)
{
    *pte |= PTE_X;
    sfence_vma();
    asm volatile("fence.i");
}

static void reset_pte_execable(pte_t *pte)
{
    *pte &= ~PTE_X;
    sfence_vma();
    asm volatile("fence.i");
}

// 在 entry_func 处改为跳到 kprobe_entry
//  auipc t0
//  jalr t0 xxx(t0)
static void set_probe_jmp_t0(void *entry_func, const void *kprobe_entry)
{
    uint32_t *patch = (uint32_t *)entry_func;
    int64_t off = (uint64_t)kprobe_entry - (uint64_t)entry_func;
    int64_t hi = (off + 0x800) >> 12;
    int64_t imm_hi = hi & 0xFFFFF;
    patch[0] = (imm_hi << 12) | (5 << 7) | 0x17;  // auipc t0, imm_hi
    int32_t imm_lo = off - (hi << 12);
    patch[1] = ((uint32_t)imm_lo << 20) | (5 << 15) | (0 << 12) | (5 << 7) | 0x67;
}

//  auipc ra
//  jalr ra xxx(t0)
static void set_probe_jmp_ra(void *entry_func, const void *kprobe_entry)
{
    uint32_t *patch = (uint32_t *)entry_func;
    int64_t off = (uint64_t)kprobe_entry - (uint64_t)entry_func;
    int64_t hi = (off + 0x800) >> 12;
    int64_t imm_hi = hi & 0xFFFFF;
    patch[0] = (imm_hi << 12) | (1 << 7) | 0x17;  // auipc ra, imm_hi
    int32_t imm_lo = off - (hi << 12);
    patch[1] = (imm_lo << 20) | (1 << 15) | (0 << 12) | (1 << 7) | 0x67;
}

static struct kprobe *kprobe_set(void *entry_func, void *my_func, void *kprobe_entry, void *kprobe_entry_set)
{
    pte_t *pte = walk(kernel_pagetable, (uint64_t)entry_func, 0);
    if (!pte) {
        printk("kprobe_set entry_func not found\n");
        return NULL;
    }
    struct kprobe *kp = alloc_kprobe();
    if (!kp)
        return NULL;

    set_pte_writable(pte);

    memcpy(kp->page, kprobe_start, (char *)kprobe_end - (char *)kprobe_start);

    // entry_func -> probe_page
    void *probe_entry = (void *)((char *)kp->page + ((char *)kprobe_entry - (char *)kprobe_start));
    memcpy(kp->src, entry_func, auipc_jalr_len);
    set_probe_jmp_t0(entry_func, probe_entry);

    // probe_entry_set -> my_func
    void *probe_entry_set = (void *)((char *)kp->page + ((char *)kprobe_entry_set - (char *)kprobe_start));
    set_probe_jmp_ra(probe_entry_set, my_func);

    // probe_page +x
    pte_t *probe_pte = walk(kernel_pagetable, (uint64_t)kp->page, 0);
    set_pte_execable(probe_pte);
    kp->entry = entry_func;

    reset_pte_writable(pte);

    return kp;
}

//! 这里限制，如果有的话，则最大支持内存 2G
struct kprobe *kprobe_exec(void *entry_func, void *my_func)
{
    struct kprobe *kp = kprobe_set(entry_func, my_func, kprobe_exec_entry, kprobe_exec_entry_set);
    if (!kp)
        return NULL;
    kp->type = EXEC_TYPE;
    return kp;
}

struct kprobe *kprobe_attch(void *entry_func, void *my_func)
{
    struct kprobe *kp = kprobe_set(entry_func, my_func, kprobe_attch_entry, kprobe_attch_entry_set);
    if (!kp)
        return NULL;
    kp->type = ATTCH_TYPE;
    return kp;
}

void kprobe_clear(struct kprobe *kp)
{
    if (!kp)
        return;
    int i;
    uint16_t *patch = (uint16_t *)kp->src;
    pte_t *pte = walk(kernel_pagetable, (uint64_t)kp->entry, 0);
    set_pte_writable(pte);
    int loop = auipc_jalr_len / sizeof(uint16_t);
    switch (kp->type) {
    case EXEC_TYPE:
        for (i = 0; i < loop; i++)
            kp->entry[i] = 0x0001;  // nop
        break;
    case ATTCH_TYPE:
        for (i = 0; i < loop; i++)
            kp->entry[i] = patch[i];
    default:
        panic("kprobe_clear type");
        break;
    }
    pte_t *probe_pte = walk(kernel_pagetable, (uint64_t)kp->page, 0);
    if (!probe_pte)
        panic("kp probe_pte is NULL\n");

    reset_pte_execable(probe_pte);
    reset_pte_writable(pte);
    free_kprobe(kp);
}
EXPORT_SYMBOL(kprobe_clear);

struct kprobe *mod_kprobe_exec(const char *symname, void *my_func)
{
    const struct kernel_symbol *ksym = lookup_symbol(symname);
    if (!ksym) {
        printk("mod_kprobe_exec: sym not find: %s\n", symname);
        return NULL;
    }
    struct kprobe *kp = kprobe_exec(ksym->addr, my_func);
    return kp;
}
EXPORT_SYMBOL(mod_kprobe_exec);

struct kprobe *mod_kprobe_attch(const char *symname, void *my_func)
{
    const struct kernel_symbol *ksym = lookup_symbol(symname);
    if (!ksym) {
        printk("mod_kprobe_exec: sym not find: %s\n", symname);
        return NULL;
    }
    struct kprobe *kp = kprobe_attch(ksym->addr, my_func);
    return kp;
}
EXPORT_SYMBOL(mod_kprobe_attch);
