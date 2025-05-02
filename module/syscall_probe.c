#include "core/export.h"
#include "core/module.h"
#include "core/proc.h"
#include "kprobe.h"
#include "mm/mm.h"
#include "riscv.h"
#include "std/stdio.h"

void my_syscall_probe()
{
    printk("my_syscall_probe\n");
    struct thread_info *p = myproc();
    printk("probe: pid:%d, sysno:%d\n", p->pid, p->tf->a7);
}

int my_probe_init()
{
    mod_kprobe_exec("syscall", my_syscall_probe);
    return 0;
}

void my_probe_exit()
{
    // mod_kprobe_exec("syscall", my_syscall_probe);
}
module_init(my_probe_init);
module_exit(my_probe_exit);
