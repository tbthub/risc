#include "core/syscall.h"

#include "core/proc.h"
#include "std/stddef.h"

extern int64 sys_debug();
extern int64 sys_fork();
extern int64 sys_exit();
extern int64 sys_wait();
extern int64 sys_pipe();
extern int64 sys_read();
extern int64 sys_kill();
extern int64 sys_exec();
extern int64 sys_fstat();
extern int64 sys_chdir();
extern int64 sys_dup();
extern int64 sys_getpid();
extern int64 sys_sbrk();
extern int64 sys_sleep();
extern int64 sys_uptime();
extern int64 sys_open();
extern int64 sys_write();
extern int64 sys_mknod();
extern int64 sys_unlink();
extern int64 sys_link();
extern int64 sys_mkdir();
extern int64 sys_close();
extern int64 sys_mmap();
extern int64 sys_munmap();
extern int64 sys_pause();
extern int64 sys_pause();
extern int64 sys_sigaction();
extern int64 sys_waitpid();
extern int64 sys_sigret();
extern int64 sys_module();

static int64 (*syscalls[])(void) = {[SYS_debug] sys_debug,   [SYS_fork] sys_fork,           [SYS_exit] sys_exit,       [SYS_wait] sys_wait,     [SYS_pipe] sys_pipe,    [SYS_read] sys_read,
                                    [SYS_kill] sys_kill,     [SYS_exec] sys_exec,           [SYS_fstat] sys_fstat,     [SYS_chdir] sys_chdir,   [SYS_dup] sys_dup,      [SYS_getpid] sys_getpid,
                                    [SYS_sbrk] sys_sbrk,     [SYS_sleep] sys_sleep,         [SYS_uptime] sys_uptime,   [SYS_open] sys_open,     [SYS_write] sys_write,  [SYS_mknod] sys_mknod,
                                    [SYS_unlink] sys_unlink, [SYS_link] sys_link,           [SYS_mkdir] sys_mkdir,     [SYS_close] sys_close,   [SYS_mmap] sys_mmap,    [SYS_munmap] sys_munmap,
                                    [SYS_pause] sys_pause,   [SYS_sigaction] sys_sigaction, [SYS_waitpid] sys_waitpid, [SYS_sigret] sys_sigret, [SYS_module] sys_module};

/*
 *  在 riscv64 中，系统调用是通过寄存器传参，具体而言
 *  依次从 a0,a1...,a6
 *  a7 存放系统调用号
 *  中断发生的时候，这些寄存器会被保存在线程的 trapframe 中。
 */

void syscall()
{
    struct thread_info *p = myproc();
    int n = p->tf->a7;
    p->tf->a0 = (int64)syscalls[n]();
}
