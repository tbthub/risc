#include "elf.h"

#include "conf.h"
#include "core/proc.h"
#include "core/vm.h"
#include "fs/file.h"
#include "lib/string.h"
#include "mm/kmalloc.h"
#include "riscv.h"

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
