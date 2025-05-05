#ifndef __USER_H__
#define __USER_H__
#include "std/stddef.h"
#include "fs/fcntl.h"

typedef void (*__sighandler_t)(int);

// 系统调用
extern int64_t fork(void);
extern int64_t exit(int) __attribute__((noreturn));
extern int64_t wait(int *);
extern int64_t pipe(int *);
extern int64_t write(int, const void *, int);
extern int64_t read(int, void *, int);
extern int64_t close(int);
extern int64_t kill(int);
extern int64_t exec(const char *, char **);
extern int64_t open(const char *, int,int);
extern int64_t mknod(const char *, short, short);
extern int64_t unlink(const char *);
// extern  int64_t fstat(int fd, struct stat *);
extern int64_t link(const char *, const char *);
extern int64_t mkdir(const char *);
extern int64_t chdir(const char *);
extern int64_t dup(int);
extern int64_t getpid(void);
extern int64_t sbrk(int);
extern int64_t sleep(int);
extern int64_t uptime(void);
extern int64_t pause(void);
extern int64_t mmap(void *addr, uint32_t len, flags64_t prot, flags_t flags, fd_t fd, uint32_t offset);

extern pid_t waitpid(pid_t pid, int *status, int options);
extern int64_t sigaction(int sig, __sighandler_t handler);

extern int64_t debug(int);

#endif
