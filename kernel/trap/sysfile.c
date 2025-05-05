#include "core/proc.h"
#include "core/signal.h"
#include "std/string.h"
#include "std/stddef.h"

// TODO
// ! 对于指针参数，不能确保有没有加入内存的情况下，在 sys_xxx 内首先可能使用 READ_ONCE(*ptr)
// ! 比如 exec 的第一个指针参数强制不能为空，需要 READ_ONCE，但第二个参数允许为空，不能 READ_ONCE
// ! 用于手动触发缺页中断，将其加载进入内存。

// ! 你也不想当持有某个自旋锁的时候发生跟缺页的中断吧。
// ! 对于指针数组, 要尽量避免在自旋锁中处理缺页

// int64_t args[] 为局部变量，存放在进程的内核栈中，如果返回，则会正常释放；不返回，比如 exit 后面会释放原来的内核栈
// exec 会继续使用原来的内核栈，直接把栈指针归位，类似于清空栈

extern int64_t do_fork();                                                               // vm.c
extern int64_t do_exec(const char *path, char *const argv[]) __attribute__((noreturn)); // trap.c
extern int64_t do_sleep(int64_t ticks);                                                   // timer.c
extern pid_t do_getpid();                                                             // proc.c
extern int64_t do_pause();                                                              // sched.c
extern int64_t do_sigaction(int sig, __sighandler_t handler);
extern int64_t do_sigret();
extern int64_t do_waitpid(pid_t pid, int *status, int options);

extern int64_t do_open(const char *path, flags_t flags, int mod);                                      // file.c
extern int64_t do_close(int fd);                                                                       // file.c
extern int64_t do_read(int fd, const void *buf, int64_t count);                                          // file.c
extern int64_t do_write(int fd, const void *buf, int64_t count);                                         // file.c
extern int64_t do_mmap(void *addr, uint32_t len, flags64_t prot, flags_t flags, fd_t fd, uint32_t offset);  // vma.c

extern int64_t do_exit(int exit_code) __attribute__((noreturn)); // 退出状态码
extern int64_t do_munmap(void *addr, uint32_t len);
extern int64_t do_wait(int *status);

extern int64_t do_pipe();
extern int64_t do_kill(int pid, int status);
extern int64_t do_fstat();
extern int64_t do_chdir(const char *path);
extern int64_t do_dup();
extern int64_t do_sbrk();
extern int64_t do_uptime();
extern int64_t do_mknod(const char *path, int mode, int dev);
extern int64_t do_unlink(const char *path);
extern int64_t do_link();
extern int64_t do_mkdir(const char *path);
extern int64_t do_module(const char * path,int mod);

// * 请确保在 trapframe 结构体中顺序放置 a0->a6
static void get_args(int64_t *args, int n) {
    struct thread_info *p = myproc();
    memcpy(args, &p->tf->a0, n * sizeof(int64_t));
}

int64_t sys_debug() {
    int64_t args[0];
    get_args(args, 1);
    panic("sys_debug!, pid:%d\n", args[0]);
    return 0;
}

int64_t sys_fork() {
    return do_fork();
}

int64_t sys_exec() {
    int64_t args[2];
    get_args(args, 2);
    READ_ONCE(*(const char *)args[0]);
    // READ_ONCE(*(char *const *)args[1]);
    return do_exec((const char *)args[0], (char *const *)args[1]);
}

__attribute__((noreturn)) int64_t sys_exit() {
    int64_t args[1];
    get_args(args, 1);
    do_exit((int)args[0]);
}

int64_t sys_wait() {
    int64_t args[1];
    get_args(args, 1);
    READ_ONCE(*(int *)args[0]);
    return do_wait((int *)args[0]);
}

int64_t sys_pipe() {
    return do_pipe();
}

int64_t sys_read() {
    int64_t args[3];
    get_args(args, 3);
    READ_ONCE(*(char *)args[1]);
    return do_read((int)args[0], (void *)args[1], args[2]);
}

int64_t sys_kill() {
    int64_t args[2];
    get_args(args, 2);
    return do_kill((int)args[0], (int)args[1]);
}

int64_t sys_fstat() {
    return do_fstat();
}

int64_t sys_chdir() {
    int64_t args[1];
    get_args(args, 1);
    READ_ONCE(*(const char *)args[0]);
    return do_chdir((const char *)args[0]);
}

int64_t sys_dup() {
    return do_dup();
}

int64_t sys_getpid() {
    return do_getpid();
}

int64_t sys_sbrk() {
    return do_sbrk();
}

int64_t sys_sleep() {
    int64_t args[1];
    get_args(args, 1);
    return do_sleep(args[0]);
}

int64_t sys_uptime() {
    return do_uptime();
}

int64_t sys_open() {
    int64_t args[3];
    get_args(args, 3);
    READ_ONCE(*(const char *)args[0]);
    return do_open((const char *)args[0], (flags_t)args[1], (int)args[3]);
}

int64_t sys_write() {
    int64_t args[3];
    get_args(args, 3);
    READ_ONCE(*(const char *)args[1]);
    return do_write((int)args[0], (const void *)args[1], args[2]);
}

int64_t sys_mknod() {
    int64_t args[3];
    get_args(args, 3);
    READ_ONCE(*(const char *)args[0]);
    return do_mknod((const char *)args[0], (int)args[1], (int)args[2]);
}

int64_t sys_unlink() {
    int64_t args[1];
    get_args(args, 1);
    READ_ONCE(*(const char *)args[0]);
    return do_unlink((const char *)args[0]);
}

int64_t sys_link() {
    return do_link();
}

int64_t sys_mkdir() {
    int64_t args[1];
    get_args(args, 1);
    READ_ONCE(*(const char *)args[0]);
    return do_mkdir((const char *)args[0]);
}

int64_t sys_close() {
    int64_t args[1];
    get_args(args, 1);
    return do_close((int)args[0]);
}

int64_t sys_mmap() {
    int64_t args[6];
    get_args(args, 6);
    return do_mmap((void *)args[0], (uint32_t)args[1], (flags64_t)args[2], (flags_t)args[3], (fd_t)args[4], (uint32_t)args[5]);
}

int64_t sys_munmap() {
    int64_t args[2];
    get_args(args, 2);
    READ_ONCE(*(char *)args[0]);
    return do_munmap((void *)args[0], (uint32_t)args[1]);
}

int64_t sys_pause() {
    return do_pause();
}

int64_t sys_sigaction() {
    int64_t args[2];
    get_args(args, 2);
    READ_ONCE(*(char *const *)args[1]);
    return do_sigaction((int)args[0], (__sighandler_t)args[1]);
}

int64_t sys_waitpid() {
    int64_t args[3];
    get_args(args, 3);
    return do_waitpid((pid_t)args[0], (int *)args[1], (int)args[2]);
}
int64_t sys_sigret()
{
    return do_sigret();
}

int64_t sys_module()
{
    int64_t args[2];
    get_args(args, 2);
    READ_ONCE(*(const char *)args[0]);
    return do_module((const char *)args[0], (int)args[1]);
}
