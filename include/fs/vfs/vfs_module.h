#ifdef __cplusplus
extern "C" {
#endif

#ifndef _VFS_MODULE_H
#define _VFS_MODULE_H

#include <stdint.h>
#include <stddef.h>
#include "vfs_process.h"
#include "vfs_io.h"

extern int vfs_open(const char *path, int flag, int mode);
extern int vfs_close(int fd);
extern int vfs_read(int fd, const void *buf, size_t size);
extern int vfs_write(int fd, void *buf, size_t size);
extern int vfs_dup(int oldfd);
extern int vfs_dup2(int oldfd, int newfd);
extern int vfs_lseek(int fd, vfs_off_t offset, int whence);
extern int vfs_mount(vfs_io_t *mountIo, void* arg);
extern int vfs_umount(const char *mountTag);

extern int vfs_chroot(const char *path);
extern char *vfs_getcwd(char *buf, size_t size);
extern int vfs_chdir(const char *path);

#endif

#ifdef __cplusplus
}
#endif
