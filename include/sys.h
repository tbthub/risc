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
int do_setup();

int k_copy_file(struct task_struct *ch);
int k_file_init(struct task_struct *task);
int k_file_deinit(struct task_struct *task);

void* k_file_mmap_init(int fd);
void k_file_mmap_close(void *ctx);
void *k_file_mmap_dup(void *ctx);
int k_file_mmap_read(void *ctx, const void *buf, size_t size);
int k_file_mmap_lseek(void *ctx, off_t offset, int whence);
int k_file_read_no_off(int fd, vfs_off_t off, void *buf, size_t size);

#endif

#ifdef __cplusplus
}
#endif
