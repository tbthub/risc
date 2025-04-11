#ifdef __cplusplus
extern "C" {
#endif


#ifndef _SYS_H
#define _SYS_H

#include "core/proc.h"
#include "vfs/vfs_io.h"
#include "std/stddef.h"
#include "fs/fcntl.h"

int do_open(const char *path, int flags, int mod);
int do_close(int fd);
int do_read(int fd, const void *buf, size_t count);
int do_write(int fd, void *buf, size_t count);
int do_lseek(int fd, off_t offset, int whence);
int do_dup(int fd);
int do_dup2(int oldfd, int newfd);

int k_copy_file(struct task_struct *ch);
int k_file_init(struct task_struct *task);
int k_file_deinit(struct task_struct *task);
#endif

#ifdef __cplusplus
}
#endif
