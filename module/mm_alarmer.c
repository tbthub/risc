#include "core/module.h"
#include "core/proc.h"
#include "core/sched.h"
#include "core/timer.h"
#include "mm/mm.h"
#include "std/stdio.h"

// 设置一个定时器，每隔 5000 时钟打印一次当前剩余内存
static int interval = 5000;
static int off = 0;

static void mm_alarmer()
{
    printk("Hello, I'm mm_alarmer. will report free mem per %d clock.\n",interval);
    struct thread_info *p = myproc();
    while (off == 1) {
        thread_timer_sleep(p, interval);
        printk("mm_alarmer: now free mem: %d pages.\n", get_free_pages());
    }
}

static int mm_alarmer_init()
{
    off = 1;
    kthread_create(get_init(), mm_alarmer, NULL, "mm_alarmer", NO_CPU_AFF);
    return 0;
}

static void mm_alarmer_exit()
{
    off = 0;
}

module_init(mm_alarmer_init);

module_exit(mm_alarmer_exit);
