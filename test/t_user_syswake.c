// #include "core/sched.h"
// #include "core/proc.h"
// #include "lib/semaphore.h"
// #include "core/timer.h"
// #include "t_head.h"

// /*
//  *
//  * 我们自定义一个系统调用，这个系统调用会陷入睡眠
//  * 然后有一个内核线程负责唤醒,内核线程每隔一段时间醒一次
//  * 查看 CPU 调度信息即可验证结果
//  *
//  */

// semaphore_t user_syswake_sem;

// static void __user_syswake_test()
// {
//     // 这里要确保用户程序跑起来后才可以唤醒
//     // 为了方便，直接让其多睡一会
//     thread_timer_sleep(myproc(), 5000);
//     while (1)
//     {
//         thread_timer_sleep(myproc(), 2000);
//         printk("wake up user program\n");
//         sem_signal(&user_syswake_sem);
//     }
// }

// void user_syswake_test()
// {
//     // if (cpuid() == 0)
//     // {
//     //     sem_init(&user_syswake_sem, 0, "xxx");
//     //     kthread_create(__user_syswake_test, NULL, "xxx", NO_CPU_AFF);
//     // }
// }