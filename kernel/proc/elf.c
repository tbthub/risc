#include "elf.h"
#include "core/proc.h"
#include "core/vm.h"
#include "conf.h"
#include "riscv.h"
#include "std/string.h"
#include "mm/kmalloc.h"
#include "fs/fcntl.h"
#include "sys.h"

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
        if (ph->type != ELF_PROG_LOAD || ph->filesz == 0)
            continue; // 只加载LOAD段（排除掉空段）

        uint64 va_start = PGROUNDDOWN(ph->vaddr);
        uint64 va_end   = PGROUNDUP(ph->vaddr + ph->memsz) - 1;

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
