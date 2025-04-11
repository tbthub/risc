#ifndef __USER_H__
#define __USER_H__
#include "std/stddef.h"

typedef void (*__sighandler_t)(int);

// 系统调用
extern int64 fork(void);
extern int64 exit(int) __attribute__((noreturn));
extern int64 wait(int *);
extern int64 pipe(int *);
extern int64 write(int, const void *, int);
extern int64 read(int, void *, int);
extern int64 close(int);
extern int64 kill(int);
extern int64 exec(const char *, char **);
extern int64 open(const char *, int);
extern int64 mknod(const char *, short, short);
extern int64 unlink(const char *);
// extern  int64 fstat(int fd, struct stat *);
extern int64 link(const char *, const char *);
extern int64 mkdir(const char *);
extern int64 chdir(const char *);
extern int64 dup(int);
extern int64 getpid(void);
extern int64 sbrk(int);
extern int64 sleep(int);
extern int64 uptime(void);
extern int64 pause(void);
extern int64 mmap(void *addr, uint32 len, flags64_t prot, flags_t flags, fd_t fd, uint32 offset);

extern pid_t waitpid(pid_t pid, int *status, int options);
extern int64 sigaction(int sig, __sighandler_t handler);

extern int64 debug(int);

#endif
