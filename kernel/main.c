// start() jumps here in supervisor mode on all CPUs.
#include "../test/t_head.h"
#include "core/locks/semaphore.h"
#include "core/proc.h"
#include "core/sched.h"
#include "core/trap.h"
#include "core/vm.h"
#include "defs.h"
#include "dev/blk/blk_dev.h"
#include "dev/cons.h"
#include "dev/plic.h"
#include "dev/uart.h"
#include "kprobe.h"
#include "lib/atomic.h"
#include "lib/fifo.h"
#include "lib/math.h"
#include "lib/string.h"
#include "mm/kmalloc.h"
#include "mm/mm.h"
#include "mm/slab.h"
#include "std/stddef.h"
#include "std/stdio.h"

volatile static int started = 0;
extern void init_s();
extern void kmods_init();
extern void mm_init();

void main()
{
    if (cpuid() == 0) {
        cons_init();
        mm_init();

        kvm_init();
        trap_init();
        plic_init();

        proc_init();
        sched_init();

        kvm_init_hart();
        trap_inithart();
        plic_inithart();
        
        init_s();
        kmods_init();

        printk("hart 0 init_s ok\n");
        printk("xv6 kernel is booting\n");

        __sync_synchronize();
        started = 1;
    }
    else {
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
