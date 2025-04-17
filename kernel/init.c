#include "../test/t_head.h"
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

// 第一个内核线程
static void init_thread(void *a)
{
    devs_init();
    work_queue_init();
    virtio_disk_init();
    efs_mount(&virtio_disk);


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

void init_s()
{
    init_t = kthread_create(NULL, init_thread, NULL, "init_t", NO_CPU_AFF);
}
