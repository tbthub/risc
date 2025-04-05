#ifdef __cplusplus
extern "C" {
#endif

#ifndef _VFS_MODULE_H
#define _VFS_MODULE_H

#include <stdint.h>
#include <stddef.h>

#include "vfs_io.h"



int vfs_open(const char *path, int flag, int mode);
int vfs_close(int fd);
int vfs_read(int fd, void *buf, size_t size);
int vfs_write(int fd, void *buf, size_t size);
int vfs_dup(int oldfd);
int vfs_dup2(int oldfd, int newfd);
int vfs_lseek(int fd, vfs_off_t offset, int whence);
int vfs_mount(vfs_io_t *mountIo);
int vfs_umount(const char *mountTag);

int vfs_chroot(const char *path);
char *vfs_getcwd(char *buf, size_t size);
int vfs_chdir(const char *path);

#endif

#ifdef __cplusplus
}
#endif