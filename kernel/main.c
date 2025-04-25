// start() jumps here in supervisor mode on all CPUs.
#include "defs.h"

#include "std/stdio.h"
#include "std/stddef.h"

#include "mm/slab.h"
#include "mm/mm.h"
#include "mm/kmalloc.h"

#include "lib/math.h"
#include "lib/fifo.h"
#include "lib/atomic.h"
#include "lib/string.h"
#include "core/locks/semaphore.h"

#include "core/vm.h"
#include "core/trap.h"
#include "core/proc.h"
#include "core/sched.h"

#include "dev/plic.h"
#include "dev/uart.h"
#include "dev/cons.h"
#include "dev/blk/blk_dev.h"

#include "../test/t_head.h"
volatile static int started = 0;
extern void init_s();

extern void mm_init();
extern int t_mem_test2();
extern int t_mem_test1();
extern int t_mem_test3();
extern int t_mem_test4();
extern void vfs_init();
void main()
{
    if (cpuid() == 0)
    {

        cons_init();
        mm_init();
        // t_mem_test4();
        // intr_off();
        // for (;;);
        
        kvm_init();
        trap_init();
        plic_init();
        proc_init();
        sched_init();
        kvm_init_hart();
        trap_inithart();
        plic_inithart();
        vfs_init();
        init_s();

        printk("hart 0 init_s ok\n");
        printk("xv6 kernel is booting\n");

        __sync_synchronize();
        started = 1;
    }
    else
    {
        while (started == 0)
            ;
        __sync_synchronize();
        kvm_init_hart();
        trap_inithart();
        plic_inithart();
    }
    __sync_synchronize();
    printk("hart %d starting\n", cpuid());

    scheduler();
}
