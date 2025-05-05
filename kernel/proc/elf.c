#include "elf.h"

#include "conf.h"
#include "core/module.h"
#include "core/proc.h"
#include "core/vm.h"
#include "fs/fcntl.h"
#include "std/string.h"
#include "mm/kmalloc.h"
#include "riscv.h"
#include "sys.h"
#include "fs/fcntl.h"

// 需要 file_lock

static int is_valid_elf(struct elf64_hdr *ehdr)
{
    return ehdr->magic == ELF_MAGIC && ehdr->machine == RISCV_MACHINE;
}
int parse_elf_header(struct elf64_hdr *ehdr, struct thread_info *t, int f) {
    struct vm_area_struct *v;
    struct proghdr *ph;
    struct mm_struct *mm = &t->task->mm;

    // 1. 基本合法性校验
    if (ehdr->magic != ELF_MAGIC)
        return -1;

    // 读出程序头表
    void *pht = kmalloc(ehdr->phnum * ehdr->phentsize, 0);
    do_lseek(f, ehdr->phoff, SEEK_SET);
    do_read(f, pht, ehdr->phnum * ehdr->phentsize);

    // 3. 遍历Program Header，加载各个段
    ph = (struct proghdr *)pht + ehdr->phnum - 1;

    for (int i = 0; i < ehdr->phnum; i++, ph--) {
        if (ph->type != ELF_PROG_LOAD || ph->memsz == 0)
            continue; // 只加载LOAD段（排除掉空段）

        uint64_t va_start = PGROUNDDOWN(ph->vaddr);
        uint64_t va_end = PGROUNDUP(ph->vaddr + ph->memsz) - 1;
        // ph->filesz == 0 但是   ph->memsz != 0，则是BSS，手动分配为0的物理页面
		
        // 不太好，但目前只能这样写 
        void *file_ctx = k_file_mmap_init(f);
        assert(file_ctx != NULL, "parse_elf_header");

        struct vm_operations_struct *vma_ops = ph->filesz == 0 ? &vma_gen_ops : &vma_file_ops;
        v = vma_alloc_proghdr(va_start, va_end, ph->flags, ph->off >> PGSHIFT, file_ctx, vma_ops);


        // v = vma_alloc_proghdr(va_start, va_end, ph->flags, ph->off >> PGSHIFT, file_ctx, &vma_file_ops);
        vma_insert(mm, v);
    }
    kfree(pht);
    return 0;
}

// @TODO
static uint32_t elf_calc_size(ElfParser *parser)
{
    uint32_t mem_sz = 0;
    struct proghdr *ph = parser->phdrs;
    for (int i = 0; i < parser->ehdr.phnum; i++, ph++) {
        if (ph->type != ELF_PROG_LOAD || ph->filesz == 0)
            continue;  // 只加载LOAD段（排除掉空段）
        // 这里包括BSS，也就是说，file_sz <= mem_sz
        mem_sz += ph->memsz;
    }
    parser->mem_sz = mem_sz;
    return mem_sz;
}

// 初始化ELF解析器
int elf_parser_init(ElfParser *parser, const char *path)
{
    parser->file = do_open(path, O_RDONLY, 0);
    if (parser->file < 0)
        return -1;

    // 读取ELF头
    k_file_read_no_off(parser->file, 0, &parser->ehdr, sizeof(struct elf64_hdr));
    if (!is_valid_elf(&parser->ehdr)) {
        do_close(parser->file);
        return -1;
    }

    // 读取程序段表
    parser->phdrs = kmalloc(sizeof(struct proghdr) * parser->ehdr.phnum, 0);
    k_file_read_no_off(parser->file, parser->ehdr.phoff, parser->phdrs, sizeof(struct proghdr) * parser->ehdr.phnum);

    // 计算文件加载大小
    elf_calc_size(parser);

    // 读取节头表
    parser->shdrs = kmalloc(sizeof(Elf64_Shdr) * parser->ehdr.shnum, 0);
    k_file_read_no_off(parser->file, parser->ehdr.shoff, parser->shdrs, sizeof(Elf64_Shdr) * parser->ehdr.shnum);

    // 读取节名字符串表
    Elf64_Shdr *shstrtab_shdr = &parser->shdrs[parser->ehdr.shstrndx];
    parser->shstrtab = kmalloc(shstrtab_shdr->sh_size, 0);
    k_file_read_no_off(parser->file, shstrtab_shdr->sh_offset, parser->shstrtab, shstrtab_shdr->sh_size);

    return 0;
}

// 解析动态链接相关节
int elf_parse_dynamic_sections(ElfParser *parser)
{
    for (int i = 0; i < parser->ehdr.shnum; i++) {
        const char *name = parser->shstrtab + parser->shdrs[i].sh_name;
        Elf64_Shdr *shdr = &parser->shdrs[i];

        if (strcmp(name, ".dynsym") == 0) {
            // printk("find dynsym\n");
            parser->dynsym = kmalloc(shdr->sh_size, 0);
            k_file_read_no_off(parser->file, shdr->sh_offset, parser->dynsym, shdr->sh_size);
        }
        else if (strcmp(name, ".dynstr") == 0) {
            // printk("find dynstr\n");
            parser->dynstr = kmalloc(shdr->sh_size, 0);
            k_file_read_no_off(parser->file, shdr->sh_offset, parser->dynstr, shdr->sh_size);
        }
        else if (strcmp(name, ".rela.dyn") == 0) {
            // printk("find rela.dyn\n");
            parser->rela_dyn = kmalloc(shdr->sh_size, 0);
            k_file_read_no_off(parser->file, shdr->sh_offset, parser->rela_dyn, shdr->sh_size);
            parser->rela_count = shdr->sh_size / sizeof(Elf64_Rela);
        }
        else if (strcmp(name, KINIT_SECTION) == 0) {
            // printk("find kmod_init\n");
            parser->mod_init_offset = shdr->sh_addr;
        }
        else if (strcmp(name, KEXIT_SECTION) == 0) {
            // printk("find kmod_exit\n");
            parser->mod_exit_offset = shdr->sh_addr;
        }
    }
    return 0;
}

// 释放ELF解析器资源
void elf_parser_destroy(ElfParser *parser)
{
    kfree(parser->phdrs);
    kfree(parser->shdrs);
    kfree(parser->shstrtab);
    kfree(parser->dynsym);
    kfree(parser->dynstr);
    kfree(parser->rela_dyn);
    do_close(parser->file);
}


inline uint32_t elf_ptload_msize(ElfParser *parser)
{
    return parser->mem_sz;
}

Elf64_Shdr *elf_find_section(ElfParser *parser, const char *name) {
    for (int i = 0; i < parser->ehdr.shnum; i++) {
        const char *sec_name = parser->shstrtab + parser->shdrs[i].sh_name;
        if (strcmp(sec_name, name) == 0) {
            return &parser->shdrs[i];
        }
    }
    return NULL;
}
