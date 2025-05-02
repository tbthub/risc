#include "../test/t_head.h"
#include "core/export.h"
#include "core/proc.h"
#include "core/sched.h"
#include "core/work.h"
#include "dev/blk/buf.h"
#include "dev/devs.h"

extern struct thread_info *init_t;

// 内存虚拟块设备
extern void mvirt_blk_dev_init();
extern void virtio_disk_init();

extern struct block_device virtio_disk;
extern int efs_mount(struct block_device *bd);
extern __attribute__((noreturn)) int do_exec(const char *path, char *const argv[]);
extern void kswapd_init();

extern pid_t do_waitpid(pid_t pid, int *status, int options);
extern int64 do_module(const char *path, int mode);

#include "core/timer.h"
#include "kprobe.h"
extern void syscall();
static void myprobe()
{
    printk("myprobe");
}

static void del_krpobe(void *arg)
{
    struct kprobe *kp = (struct kprobe *)arg;
    printk("del_krpobe\n");
    kprobe_clear(kp);
}

// 第一个内核线程
static void init_thread(void *a)
{
    devs_init();
    work_queue_init();
    virtio_disk_init();
    efs_mount(&virtio_disk);

#ifdef CONF_MKMOD
    // do_module("/mm_alarmer", 1);
    // do_module("/sys_probe", 1);
    // while (1) {
    //     thread_timer_sleep(myproc(), 100);
    // }
#endif

    struct kprobe *kp = kprobe_exec(syscall, myprobe);
    timer_create(del_krpobe, kp, 1000, 1, TIMER_NO_BLOCK);
    
#ifdef CONF_MKFS
    mkfs_tmp_test();
    while (1) {
        thread_timer_sleep(myproc(), 100);
    }
#endif

    kswapd_init();
    struct thread_info *p = myproc();
    // 在设置 tf 之前都是位于直接使用的内核页表，
    spin_lock(&p->lock);
    alloc_user_pgd(&p->task->mm);
    p->tf = kmem_cache_alloc(&tf_kmem_cache);
    spin_unlock(&p->lock);

    intr_off();
    do_exec("/init", NULL);
}

extern void user_init();
void init_s()
{
    user_init();
    init_t = kthread_create(NULL, init_thread, NULL, "init_t", NO_CPU_AFF);
}
