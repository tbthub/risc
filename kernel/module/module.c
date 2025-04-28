#include "core/module.h"

#include "core/export.h"
#include "core/locks/spinlock.h"
#include "core/proc.h"
#include "core/vm.h"
#include "elf.h"
#include "fs/file.h"
#include "lib/list.h"
#include "lib/string.h"
#include "mm/kmalloc.h"
#include "mm/mm.h"
#include "std/stddef.h"

#define MOD_MAX_TOP 0x80000000
#define MOD_BASE 0x70000000

// 内核模块管理器
struct
{
    spinlock_t lock;
    uint32 cnt;
    struct list_head list;
    uint64 next_base;
} Kmods;

// 内核模块
struct kmod
{
    char *km_name;
    struct list_head km_list;
    struct file *km_f;
    uint64 km_base;
    initcall_t km_init;
    exitcall_t km_exit;
};

static struct kmod *alloc_kmod()
{
    struct kmod *km = kmalloc(sizeof(struct kmod), 0);
    INIT_LIST_HEAD(&km->km_list);
    return km;
}

// 分配虚存空间
static uint64 alloc_mod_space(uint32 mem_size)
{
    uint64 base;
    spin_lock(&Kmods.lock);
    base = Kmods.next_base;
    if (Kmods.next_base + mem_size >= MOD_MAX_TOP) {
        spin_unlock(&Kmods.lock);
        panic("alloc_mod_space\n");
        return -1;
    }
    Kmods.next_base = Kmods.next_base + mem_size + 50 * PGSIZE;
    // printk("Kmods.next_base: %p\n",Kmods.next_base);
    spin_unlock(&Kmods.lock);
    return base;
}

// static void apply_relocations(void *module_base, Elf64_Shdr *rela_shdr, const char *strtab)
// {

// }

// static initcall_t find_init_sym(struct elf64_hdr *ehd, struct file *f)
// {
//     // find sys = MOD_INIT_SYM -> entry
// }

static int __insmod(struct kmod *km, struct file *f, uint32 mem_size, uint32 pgoff)
{
    uint64 base;

    struct vm_area_struct *v;
    struct thread_info *t = myproc();

    if ((base = alloc_mod_space(mem_size)) == -1) {
        printk("Kmod vm base\n");
        return -1;
    }

    v = vma_alloc_proghdr(PGROUNDDOWN(base), PGROUNDUP(base + mem_size), ELF_PROG_FLAG_EXEC | ELF_PROG_FLAG_READ | ELF_PROG_FLAG_WRITE, pgoff, f, &vma_file_ops);
    vma_insert(&t->task->mm, v);
    return 0;
}

extern pagetable_t kernel_pagetable;
extern int mappages(pagetable_t pagetable, uint64 va, uint64 pa, uint64 size, int perm);
extern void elf_kmod_half1(struct file *f, struct elf64_hdr *ehdr, uint32 *mem_sz, uint32 *off);

static int insmod(const char *path)
{
    int res;
    uint32 off = 0;
    uint32 mem_sz = 0;
    struct elf64_hdr *ehd;
    struct file *f;

    ehd = kmalloc(sizeof(struct elf64_hdr), 0);
    f = file_open(path, FILE_READ);
    if (!f)
        goto file_error;
    struct kmod *km = alloc_kmod();
    if (!km)
        goto alloc_error;
    file_lock(f);
    read_elf(ehd, f);
    elf_kmod_half1(f, ehd, &mem_sz, &off);

    // 1. 读取节头表
    Elf64_Shdr *shdrs = kmalloc(sizeof(Elf64_Shdr) * ehd->shnum, 0);
    file_read_no_off(f, ehd->shoff, shdrs, sizeof(Elf64_Shdr) * ehd->shnum);

    // 2. 获取节名字符串表（.shstrtab）的节头
    int strtab_idx = ehd->shstrndx;
    // printk("A: %d\n", ehd->shstrndx);
    Elf64_Shdr *strtab_shdr = &shdrs[strtab_idx];

    // 3. 分配并读取字符串表内容
    char *shstrtab = kmalloc(strtab_shdr->sh_size, 0);
    file_read_no_off(f, strtab_shdr->sh_offset, shstrtab, strtab_shdr->sh_size);

    Elf64_Sym *dynsym = NULL;     // 动态符号表
    char *dynstr = NULL;          // 动态符号字符串表
    Elf64_Rela *rela_dyn = NULL;  // 动态重定位表
    uint32 rela_count = 0;        // 重定位条目数量
    Elf64_Addr *got = NULL;       // 全局偏移表
    uint64 mod_init_func = 0;
    uint64 mod_exit_func = 0;

    // 遍历所有节头
    for (int i = 0; i < ehd->shnum; i++) {
        const char *name = shstrtab + shdrs[i].sh_name;
        if (strcmp(name, ".dynsym") == 0) {
            printk("find dynsym\n");
            dynsym = kmalloc(shdrs[i].sh_size, 0);
            file_read_no_off(f, shdrs[i].sh_offset, dynsym, shdrs[i].sh_size);
        }
        else if (strcmp(name, ".dynstr") == 0) {
            printk("find dynstr\n");
            dynstr = kmalloc(shdrs[i].sh_size, 0);
            file_read_no_off(f, shdrs[i].sh_offset, dynstr, shdrs[i].sh_size);
        }
        else if (strcmp(name, ".rela.dyn") == 0) {
            printk("find rela.dyn\n");
            rela_dyn = kmalloc(shdrs[i].sh_size, 0);
            file_read_no_off(f, shdrs[i].sh_offset, rela_dyn, shdrs[i].sh_size);
            rela_count = shdrs[i].sh_size / sizeof(Elf64_Rela);
        }
        else if (strcmp(name, ".got") == 0) {
            printk("find got\n");
            got = kmalloc(shdrs[i].sh_size, 0);
            file_read_no_off(f, shdrs[i].sh_offset, got, shdrs[i].sh_size);
        }
        else if (strcmp(name, INIT_SECTION) == 0) {
            printk("find mod_init\n");
            // mod_init_func = shdrs[i].sh_offset;
            mod_init_func = shdrs[i].sh_addr; 
        }
        else if (strcmp(name, EXIT_SECTION) == 0) {
            printk("find mod_exit\n");
            mod_exit_func = shdrs[i].sh_addr; 
        }
    }

    uint64 module_base = 0;
    if ((module_base = alloc_mod_space(mem_sz)) == -1) {
        printk("Kmod vm base\n");
        return -1;
    }

    void *context = __alloc_pages(0, 6);  // 测试
    file_read_no_off(f, off, context, mem_sz);
    mappages(kernel_pagetable, module_base, (uint64)context, 32 * PGSIZE, PTE_R | PTE_W | PTE_X);
    sfence_vma();

    printk("rela_count: %d\n", rela_count);
    // 遍历所有重定位条目
    for (int32 i = 0; i < rela_count; i++) {
        Elf64_Rela *rela = &rela_dyn[i];
        uint32 type = ELF64_R_TYPE(rela->r_info);

        switch (type) {
        case R_RISCV_JUMP_SLOT: {
            // 动态符号重定位（需要查符号表）
            uint32 sym_idx = ELF64_R_SYM(rela->r_info);
            Elf64_Sym *sym = &dynsym[sym_idx];
            const char *symname = dynstr + sym->st_name;

            const struct kernel_symbol *ksym = lookup_symbol(symname);
            if (!ksym) {
                printk("Failed to resolve symbol: %s\n", symname);
                return -1;
            }

            void *target = (void *)(module_base + rela->r_offset);
            *(uint64 *)target = (uint64)ksym->addr + rela->r_addend;
            break;
        }
        case R_RISCV_RELATIVE: {
            // 基地址重定位（无需符号）
            void *target = (void *)(module_base + rela->r_offset);
            *(uint64 *)target = module_base + rela->r_addend;  // 关键计算
            break;
        }
        default:
            printk("Unsupported relocation type: %d\n", type);
            return -1;
        }
    }

    // panic("aa\n");

    // 查找 init_module 和 cleanup_module
    initcall_t *init_func = (initcall_t*)(module_base + mod_init_func);
    exitcall_t *exit_func = (exitcall_t*)(module_base + mod_exit_func);

    struct kmod *mod = kmalloc(sizeof(struct kmod), 0);
    mod->km_name = "example_module";
    mod->km_f = f;  // 假设 f 是模块文件指针
    mod->km_base = module_base;
    mod->km_init = *init_func;
    mod->km_exit = *exit_func;

    mod->km_init();
    mod->km_exit();

    // 完成动态链接后，再释放内存
    kfree(shdrs);
    kfree(shstrtab);

    // file_read_no_off(f, ehd->phoff, pht, ehd->phnum * ehd->phentsize);

    res = __insmod(km, f, mem_sz, off >> PGSHIFT);

    // panic("insmod\n");
    return res;

alloc_error:
    file_close(f);

file_error:
    return -1;
}

static int rmmod(const char *path)
{
    return -1;
}

void kmodule_init()
{
    spin_init(&Kmods.lock, "Kmods");
    Kmods.cnt = 0;
    INIT_LIST_HEAD(&Kmods.list);
    Kmods.next_base = MOD_BASE;
}

int64 do_module(const char *path, int mode)
{
    int res = -1;
    switch (mode) {
    case IN_MOD:
        res = insmod(path);
        break;

    case RM_MOD:
        res = rmmod(path);
        break;

    default:
        break;
    }
    return res;
}
