#include "std/stddef.h"
#include "core/proc.h"
#include "fs/file.h"
#include "elf.h"
#include "core/proc.h"
#include "core/vm.h"
#include "fs/file.h"
#include "conf.h"

extern int do_fork();
extern int do_exec(const char *path, char *const argv[]) __attribute__((noreturn)); // trap.c
extern int do_sleep(uint64_t ticks);                                                  // timer.c
extern int do_wait(int *status);
extern int do_pipe();
extern int do_kill(int pid, int status);
extern int do_fstat();
extern int do_chdir(const char *path);
extern int do_dup();
extern int do_getpid(); // proc.c
extern int do_sbrk();
extern int do_uptime();
extern int do_open(const char *path, flags_t flags,int mod);
extern int do_close(int fd);
extern int do_read(int fd, void *buf, uint64_t count);
extern int do_write(int fd, const void *buf, uint64_t count);
extern int do_mknod(const char *path, int mode, int dev);
extern int do_unlink(const char *path);
extern int do_link();
extern int do_mkdir(const char *path);

int do_wait(int *status)
{
    return -ENOSYS;
}

int do_pipe()
{
    return -ENOSYS;
}

int do_kill(int pid, int status)
{
    return -ENOSYS;
}

int do_fstat()
{
    return -ENOSYS;
}

int do_chdir(const char *path)
{
    return -ENOSYS;
}

int do_dup()
{
    return -ENOSYS;
}

int do_sbrk()
{
    return -ENOSYS;
}

int do_uptime()
{
    return -ENOSYS;
}


int do_mknod(const char *path, int mode, int dev)
{
    return -ENOSYS;
}

int do_unlink(const char *path)
{
    return -ENOSYS;
}

int do_link()
{
    return -ENOSYS;
}

int do_mkdir(const char *path)
{
    return -ENOSYS;
}


