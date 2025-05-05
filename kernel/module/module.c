#include "core/module.h"

#include "core/export.h"
#include "core/locks/spinlock.h"
#include "core/proc.h"
#include "core/vm.h"
#include "elf.h"
#include "fs/file.h"
#include "lib/hash.h"
#include "lib/list.h"
#include "lib/string.h"
#include "mm/kmalloc.h"
#include "mm/mm.h"
#include "std/stddef.h"

#define MOD_MAX_TOP 0x80000000
#define MOD_BASE 0x70000000

#define KMODS_HASN_SIZE 250

// 内核模块管理器
static struct
{
    spinlock_t lock;
    int32 cnt;
    struct list_head list;
    uint64_t next_base;

    struct hash_table ht;
} Kmods;

// 内核模块
struct kmod
{
    char km_name[20];
    struct list_head km_list;

    uint64_t km_base;
    uint32_t km_size;

    initcall_t km_init;
    exitcall_t km_exit;
    ElfParser *km_parser;
};

extern pagetable_t kernel_pagetable;
extern int mappages(pagetable_t pagetable, uint64_t va, uint64_t pa, uint64_t size, int perm);
extern struct ksym *alloc_ksym(struct kernel_symbol *sym);
extern uint32_t ksym_hash(struct ksym *ks);

// TODO 临时为了内核模块探针调试 导出下
const struct kernel_symbol *lookup_symbol(const char *name)
{
    uint32_t key = strhash(name);
    struct ksym *ks = NULL;
    spin_lock(&Kmods.lock);
    hash_for_each_entry(ks, &Kmods.ht, key, node)
    {
        if (strcmp(name, ks->ksp->name) == 0) {
            spin_unlock(&Kmods.lock);
            return ks->ksp;
        }
    }
    spin_unlock(&Kmods.lock);
    return NULL;
}
EXPORT_SYMBOL(lookup_symbol);

static void kmod_add_global(struct kmod *kmod)
{
    spin_lock(&Kmods.lock);
    list_add_head(&kmod->km_list, &Kmods.list);
    Kmods.cnt++;
    spin_unlock(&Kmods.lock);
}

static struct kmod *find_module(const char *path)
{
    struct kmod *k = NULL;
    spin_lock(&Kmods.lock);
    list_for_each_entry(k, &Kmods.list, km_list)
    {
        if (strcmp(k->km_name, path) == 0) {
            spin_unlock(&Kmods.lock);
            return k;
        }
    }
    spin_unlock(&Kmods.lock);
    return NULL;
}

static void kmod_rm_global(struct kmod *k)
{
    spin_lock(&Kmods.lock);
    list_del(&k->km_list);
    Kmods.cnt--;
    spin_unlock(&Kmods.lock);
}

// 创建内核模块对象
static struct kmod *kmod_create(const char *name, ElfParser *parser)
{
    struct kmod *km = kmalloc(sizeof(struct kmod), 0);
    strncpy(km->km_name, name, 20);
    km->km_parser = parser;
    INIT_LIST_HEAD(&km->km_list);
    return km;
}

static inline void kmod_destroy(struct kmod *kmod)
{
    kfree(kmod);
}

// 分配虚存空间
static uint64_t alloc_mod_space(uint32_t mem_size)
{
    uint64_t base;
    spin_lock(&Kmods.lock);
    base = Kmods.next_base;
    if (Kmods.next_base + mem_size >= MOD_MAX_TOP) {
        spin_unlock(&Kmods.lock);
        panic("alloc_mod_space\n");
        return -1;
    }
    Kmods.next_base = PGROUNDUP(Kmods.next_base + mem_size + PGSIZE);
    spin_unlock(&Kmods.lock);
    return base;
}

// 分配模块内存空间
static int kmod_alloc_memory(struct kmod *km)
{
    // 计算需要的内存大小（这里可以更精确）
    // ! 注意 这里实际上包含了 bss 段的大小
    km->km_size = elf_ptload_msize(km->km_parser);
    km->km_base = alloc_mod_space(km->km_size);
    return (km->km_base != -1) ? 0 : -1;
}

// 加载模块代码到内存
static int kmod_load_code(struct kmod *km)
{
    Elf64_Shdr *code_shdr = elf_find_section(km->km_parser, KINIT_SECTION);
    if (!code_shdr) {
        printk("Module %s section not found\n", KINIT_SECTION);
        return -1;
    }
    uint64_t code_offset = code_shdr->sh_offset;

    for (uint64_t va = 0; va < km->km_size; va += PGSIZE) {
        // 分配物理页
        void *context = __alloc_page(0);
        if (!context) {
            printk("Failed to alloc page\n");
            return -1;
        }
        // 计算当前页读取大小
        int sz = min(km->km_size - va, PGSIZE);
        // 从文件读取代码到物理页
        file_read_no_off(km->km_parser->file, code_offset + va, context, sz);
        mappages(kernel_pagetable, km->km_base + va, (uint64_t)context, PGSIZE, PTE_R | PTE_W | PTE_X);
    }
    sfence_vma();
    return 0;
}

// 应用重定位
static int kmod_apply_relocations(struct kmod *km)
{
    ElfParser *p = km->km_parser;
    for (int i = 0; i < p->rela_count; i++) {
        Elf64_Rela *rela = &p->rela_dyn[i];
        uint32_t type = ELF64_R_TYPE(rela->r_info);

        switch (type) {
        case R_RISCV_JUMP_SLOT: {
            uint32_t sym_idx = ELF64_R_SYM(rela->r_info);
            Elf64_Sym *sym = &p->dynsym[sym_idx];
            const char *symname = p->dynstr + sym->st_name;

            const struct kernel_symbol *ksym = lookup_symbol(symname);
            // printk("find sym: %s,ok -> %p\n",ksym->name,ksym->addr);
            if (!ksym) {
                panic("sym not find: %s\n", symname);
                return -1;
            }

            *(uint64_t *)(km->km_base + rela->r_offset) = (uint64_t)ksym->addr + rela->r_addend;
            break;
        }
        case R_RISCV_RELATIVE:
            *(uint64_t *)(km->km_base + rela->r_offset) = km->km_base + rela->r_addend;
            break;
        case R_RISCV_64:
            // 处理 64 位绝对地址重定位
            uint32_t sym_idx = ELF64_R_SYM(rela->r_info);
            Elf64_Sym *sym = &p->dynsym[sym_idx];
            uint64_t value;

            if (sym->st_shndx == SHN_UNDEF) {
                // 外部符号：查找内核符号表
                const char *symname = p->dynstr + sym->st_name;
                const struct kernel_symbol *ksym = lookup_symbol(symname);
                if (!ksym) {
                    panic("Undefined symbol: %s\n", symname);
                    return -1;
                }
                value = (uint64_t)ksym->addr;
            }
            else {
                // 内部符号：计算模块内地址
                value = km->km_base + sym->st_value;
            }

            // 写入重定位地址 = 符号地址 + addend
            *(uint64_t *)(km->km_base + rela->r_offset) = value + rela->r_addend;
            break;
        default:
            panic("undeal type: %d\n", type);
            return -1;
        }
    }
    return 0;
}

// 初始化模块
static int kmod_initialize(struct kmod *km)
{
    km->km_init = *(initcall_t *)(km->km_base + km->km_parser->mod_init_offset);
    km->km_exit = *(exitcall_t *)(km->km_base + km->km_parser->mod_exit_offset);
    return 0;
}

static int insmod(const char *path)
{
    // 阶段1：ELF解析
    ElfParser parser;
    struct kmod *mod = NULL;
    if (elf_parser_init(&parser, path) < 0)
        return -1;
    if (elf_parse_dynamic_sections(&parser) < 0)
        goto error;

    // 阶段2：模块管理
    mod = kmod_create(path, &parser);
    if (kmod_alloc_memory(mod) < 0)
        goto error;
    if (kmod_load_code(mod) < 0)
        goto error;
    if (kmod_apply_relocations(mod) < 0)
        goto error;
    if (kmod_initialize(mod) < 0)
        goto error;

    kmod_add_global(mod);
    // 清理资源
    elf_parser_destroy(&parser);

    mod->km_init();
    return 0;

error:
    elf_parser_destroy(&parser);
    kmod_destroy(mod);
    return -1;
}

static int rmmod(const char *path)
{
    struct kmod *k = find_module(path);
    if (!k)
        return 0;
    kmod_rm_global(k);
    k->km_exit();
    // 1. 根据 path 寻找到对应的 kmod
    // 2. 执行 km_exit()
    // 3. 回收虚存映射
    return 0;
}

static void kmods_hash_init()
{
    struct ksym *ks = NULL;
    hash_init(&Kmods.ht, KMODS_HASN_SIZE, "Kmods");
    for (struct kernel_symbol *sym = __start___ksymtab; sym < __stop___ksymtab; sym++) {
        ks = alloc_ksym(sym);
        hash_add_head(&Kmods.ht, ksym_hash(ks), &ks->node);
    }
}

// const struct kernel_symbol *lookup_symbol(const char *name) {
//     for (struct kernel_symbol *sym = __start___ksymtab;
//          sym < __stop___ksymtab;
//          sym++) {
//         if (strcmp(sym->name, name) == 0) {
//             return sym;
//         }
//     }
//     return NULL;
// }

void kmods_init()
{
    spin_init(&Kmods.lock, "Kmods");
    Kmods.cnt = 0;
    INIT_LIST_HEAD(&Kmods.list);
    // TODO 我们做了取巧，直接把基地址映射，这样顶层页表就会有对应条目
    void *no_use_page = __alloc_page(0);
    mappages(kernel_pagetable, MOD_BASE, (uint64_t)no_use_page, PGSIZE, 0);
    Kmods.next_base = MOD_BASE + 2 * PGSIZE;

    kmods_hash_init();
    // 执行所有的初始化函数
    initcall_t *k_inits;
    for (k_inits = (initcall_t *)kmod_init_start; k_inits < (initcall_t *)kmod_init_end; k_inits++)
        (*k_inits)();
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
