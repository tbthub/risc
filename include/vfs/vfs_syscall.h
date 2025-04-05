// #ifdef __cplusplus
// extern "C" {
// #endif

// #ifndef _VFS_CALL_H
// #define _VFS_CALL_H

// #include "vfs/vfs_interface.h"
// #include "std/stdint.h"
// #include "std/stddef.h"

// typedef int mode_t;
// typedef int uid_t;
// typedef int gid_t;
// typedef long off_t;

// // 文件相关
// int sys_close(int fd);
// ssize_t sys_read(int fd, void *buf, size_t count);
// ssize_t sys_write(int fd, const void *buf, size_t count);
// off_t sys_seek(int fd, off_t offset, int whence);
// int sys_flush(int fd);
// int sys_open(const char *pathname, int flags, mode_t mode);

// // 文件系统操作
// int sys_chdir(const char *path);
// int sys_chmod(const char *path, mode_t mode);
// int sys_chown(const char *path, uid_t owner, gid_t group);
// int sys_chroot(const char *path);
// int sys_mkdir(const char *pathname, mode_t mode);
// int sys_rmdir(const char *pathname);

// // 文件描述符操作
// int sys_dup(int oldfd);
// int sys_dup2(int oldfd, int newfd);
// int sys_dup3(int oldfd, int newfd, int flags);

// // 系统信息
// int sys_stat(const char *path, void *buf); // 使用结构体指针需自定义
// char *sys_getcwd(char *buf, size_t size);

// // 高级IO控制
// int sys_fcntl(int fd, int cmd, ... /* arg */);
// int sys_epoll_create(int size);

// // 设备管理
// int sys_mount(const char *source, const char *target, const char *filesystemtype, unsigned long mountflags, const void *data);
// int sys_umount(const char *target);

// int kmount_simfs(const char *tag);

// #endif

// #ifdef __cplusplus
// }
// #endif
