#include "elf.h"

#include "conf.h"
#include "core/proc.h"
#include "core/vm.h"
#include "fs/file.h"
#include "lib/string.h"
#include "mm/kmalloc.h"
#include "riscv.h"

// 需要 file_lock

int parse_elf_header(struct elf64_hdr *ehdr, struct thread_info *t, struct file *f)
{
    struct vm_area_struct *v;
    struct proghdr *ph;
    struct mm_struct *mm = &t->task->mm;

    // 1. 基本合法性校验
    if (ehdr->magic != ELF_MAGIC)
        return -1;

    // 读出程序头表
    void *pht = kmalloc(ehdr->phnum * ehdr->phentsize, 0);
    file_llseek(f, ehdr->phoff, SEEK_SET);
    file_read(f, pht, ehdr->phnum * ehdr->phentsize);

    // 3. 遍历Program Header，加载各个段
    ph = (struct proghdr *)pht + ehdr->phnum - 1;

    for (int i = 0; i < ehdr->phnum; i++, ph--) {
        if (ph->type != ELF_PROG_LOAD || ph->memsz == 0)
            continue;  // 只加载LOAD段（排除掉空段）
        uint64 va_start = PGROUNDDOWN(ph->vaddr);
        uint64 va_end = PGROUNDUP(ph->vaddr + ph->memsz) - 1;
        // ph->filesz == 0 但是   ph->memsz != 0，则是BSS，手动分配为0的物理页面
        struct vm_operations_struct *vma_ops = ph->filesz == 0 ? &vma_gen_ops : &vma_file_ops;
        v = vma_alloc_proghdr(va_start, va_end, ph->flags, ph->off >> PGSHIFT, f, vma_ops);
        vma_insert(mm, v);
    }
    kfree(pht);
    return 0;
}

int read_elf(struct elf64_hdr *ehdr, struct file *f)
{
    file_read_no_off(f, 0, ehdr, sizeof(struct elf64_hdr));
    if (ehdr->magic != ELF_MAGIC)
        panic("read_elf\n");
    return 0;
}

int elf_first_segoff(struct elf64_hdr *ehdr, struct file *f)
{
    // struct proghdr *ph;
    // int off = -1;
    // void *pht = kmalloc(ehdr->phnum * ehdr->phentsize, 0);
    // file_read_no_off(f, ehdr->phoff, pht, ehdr->phnum * ehdr->phentsize);
    // ph = (struct proghdr *)pht + ehdr->phnum - 1;
    // for (int i = 0; i < ehdr->phnum; i++, ph--) {
    //     if (ph->type != ELF_PROG_LOAD || ph->memsz == 0)
    //         continue;  // 只加载LOAD段（排除掉空段）
    //     if ((ph->flags & ELF_PROG_FLAG_EXEC) != 0) {
    //         off = ph->off;
    //         break;
    //     }
    // }
    // kfree(pht);
    // return off;
    return 0;
}

int elf_load_mmsz(struct elf64_hdr *ehdr, struct file *f)
{
    // struct proghdr *ph;
    // uint32 memsz = 0;
    // void *pht = kmalloc(ehdr->phnum * ehdr->phentsize, 0);
    // file_read_no_off(f, ehdr->phoff, pht, ehdr->phnum * ehdr->phentsize);
    // ph = (struct proghdr *)pht + ehdr->phnum - 1;
    // for (int i = 0; i < ehdr->phnum; i++, ph--) {
    //     if (ph->type != ELF_PROG_LOAD || ph->memsz == 0)
    //         continue;  // 只加载LOAD段（排除掉空段）
    //     memsz += ph->memsz;
    // }
    // kfree(pht);
    // return memsz;
    return 0;
}

void elf_kmod_half1(struct file *f,struct elf64_hdr *ehdr,uint32 *mem_sz,uint32 *off)
{
    struct proghdr *ph;
    void *pht = kmalloc(ehdr->phnum * ehdr->phentsize, 0);
    file_read_no_off(f, ehdr->phoff, pht, ehdr->phnum * ehdr->phentsize);
    // 对与模块，我们规定代码段一定是在第一个加载段，参见 mod.ld 脚本
    ph = (struct proghdr *)pht + ehdr->phnum - 1;
    for (int i = 0; i < ehdr->phnum; i++, ph--) {
        if (ph->type != ELF_PROG_LOAD || ph->memsz == 0)
            continue;  // 只加载LOAD段（排除掉空段）
        *mem_sz += ph->memsz;
        if ((ph->flags & ELF_PROG_FLAG_EXEC) != 0) {
            *off = ph->off;
        }
    }
    kfree(pht);
}
