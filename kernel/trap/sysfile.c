#include "core/proc.h"
#include "core/signal.h"
#include "fs/file.h"
#include "lib/string.h"
#include "std/stddef.h"

// TODO
// ! 对于指针参数，不能确保有没有加入内存的情况下，在 sys_xxx 内首先可能使用 READ_ONCE(*ptr)
// ! 比如 exec 的第一个指针参数强制不能为空，需要 READ_ONCE，但第二个参数允许为空，不能 READ_ONCE
// ! 用于手动触发缺页中断，将其加载进入内存。

// ! 你也不想当持有某个自旋锁的时候发生跟缺页的中断吧。
// ! 对于指针数组, 要尽量避免在自旋锁中处理缺页

// int64 args[] 为局部变量，存放在进程的内核栈中，如果返回，则会正常释放；不返回，比如 exit 后面会释放原来的内核栈
// exec 会继续使用原来的内核栈，直接把栈指针归位，类似于清空栈

extern int64 do_fork();                                                                // vm.c
extern int64 do_exec(const char *path, char *const argv[]) __attribute__((noreturn));  // trap.c
extern int64 do_sleep(int64 ticks);                                                    // timer.c
extern pid_t do_getpid();                                                              // proc.c
extern int64 do_pause();                                                               // sched.c
extern int64 do_sigaction(int sig, __sighandler_t handler);
extern int64 do_sigret();
extern int64 do_waitpid(pid_t pid, int *status, int options);

extern int64 do_open(const char *path, flags_t flags, int mod);                                       // file.c
extern int64 do_close(int fd);                                                                        // file.c
extern int64 do_read(int fd, const void *buf, int64 count);                                           // file.c
extern int64 do_write(int fd, const void *buf, int64 count);                                          // file.c
extern int64 do_mmap(void *addr, uint32 len, flags64_t prot, flags_t flags, fd_t fd, uint32 offset);  // vma.c

extern int64 do_exit(int exit_code) __attribute__((noreturn));  // 退出状态码
extern int64 do_munmap(void *addr, uint32 len);
extern int64 do_wait(int *status);

extern int64 do_pipe();
extern int64 do_kill(int pid, int status);
extern int64 do_fstat();
extern int64 do_chdir(const char *path);
extern int64 do_dup();
extern int64 do_sbrk();
extern int64 do_uptime();
extern int64 do_mknod(const char *path, int mode, int dev);
extern int64 do_unlink(const char *path);
extern int64 do_link();
extern int64 do_mkdir(const char *path);

// * 请确保在 trapframe 结构体中顺序放置 a0->a6
static void get_args(int64 *args, int n)
{
    struct thread_info *p = myproc();
    memcpy(args, &p->tf->a0, n * sizeof(int64));
}

int64 sys_debug()
{
    int64 args[0];
    get_args(args, 1);
    // panic("sys_debug!, pid:%d\n", args[0]);
    printk("sys_debug %p\n",args[0]);
    return 0;
}

int64 sys_fork()
{
    return do_fork();
}

int64 sys_exec()
{
    int64 args[2];
    get_args(args, 2);
    READ_ONCE(*(const char *)args[0]);
    // READ_ONCE(*(char *const *)args[1]);
    return do_exec((const char *)args[0], (char *const *)args[1]);
}

__attribute__((noreturn)) int64 sys_exit()
{
    int64 args[1];
    get_args(args, 1);
    do_exit((int)args[0]);
}

int64 sys_wait()
{
    int64 args[1];
    get_args(args, 1);
    READ_ONCE(*(int *)args[0]);
    return do_wait((int *)args[0]);
}

int64 sys_pipe()
{
    return do_pipe();
}

int64 sys_read()
{
    int64 args[3];
    get_args(args, 3);
    READ_ONCE(*(char *)args[1]);
    return do_read((int)args[0], (void *)args[1], args[2]);
}

int64 sys_kill()
{
    int64 args[2];
    get_args(args, 2);
    return do_kill((int)args[0], (int)args[1]);
}

int64 sys_fstat()
{
    return do_fstat();
}

int64 sys_chdir()
{
    int64 args[1];
    get_args(args, 1);
    READ_ONCE(*(const char *)args[0]);
    return do_chdir((const char *)args[0]);
}

int64 sys_dup()
{
    return do_dup();
}

int64 sys_getpid()
{
    return do_getpid();
}

int64 sys_sbrk()
{
    return do_sbrk();
}

int64 sys_sleep()
{
    int64 args[1];
    get_args(args, 1);
    return do_sleep(args[0]);
}

int64 sys_uptime()
{
    return do_uptime();
}

int64 sys_open()
{
    int64 args[3];
    get_args(args, 3);
    READ_ONCE(*(const char *)args[0]);
    return do_open((const char *)args[0], (flags_t)args[1], (int)args[3]);
}

int64 sys_write()
{
    int64 args[3];
    get_args(args, 3);
    READ_ONCE(*(const char *)args[1]);
    return do_write((int)args[0], (const void *)args[1], args[2]);
}

int64 sys_mknod()
{
    int64 args[3];
    get_args(args, 3);
    READ_ONCE(*(const char *)args[0]);
    return do_mknod((const char *)args[0], (int)args[1], (int)args[2]);
}

int64 sys_unlink()
{
    int64 args[1];
    get_args(args, 1);
    READ_ONCE(*(const char *)args[0]);
    return do_unlink((const char *)args[0]);
}

int64 sys_link()
{
    return do_link();
}

int64 sys_mkdir()
{
    int64 args[1];
    get_args(args, 1);
    READ_ONCE(*(const char *)args[0]);
    return do_mkdir((const char *)args[0]);
}

int64 sys_close()
{
    int64 args[1];
    get_args(args, 1);
    return do_close((int)args[0]);
}

int64 sys_mmap()
{
    int64 args[6];
    get_args(args, 6);
    return do_mmap((void *)args[0], (uint32)args[1], (flags64_t)args[2], (flags_t)args[3], (fd_t)args[4], (uint32)args[5]);
}

int64 sys_munmap()
{
    int64 args[2];
    get_args(args, 2);
    READ_ONCE(*(char *)args[0]);
    return do_munmap((void *)args[0], (uint32)args[1]);
}

int64 sys_pause()
{
    return do_pause();
}

int64 sys_sigaction()
{
    int64 args[2];
    get_args(args, 2);
    READ_ONCE(*(char *const *)args[1]);
    return do_sigaction((int)args[0], (__sighandler_t)args[1]);
}

int64 sys_waitpid()
{
    int64 args[3];
    get_args(args, 3);
    return do_waitpid((pid_t)args[0], (int *)args[1], (int)args[2]);
}

int64 sys_sigret()
{
    return do_sigret();
}
