#include "vfs/vfs_interface.h"
#include "vfs/vfs_io.h"
#include "vfs_syscall.h"
#include "vfs/vfs_module.h"
#include "driver/fs/simfs/simfs.h"
#include <string.h>

int kmount_simfs(const char *tag) {
    vfs_io_t io_table;

    if (tag == NULL) {
        return -1;
    }

    simfs_io_table_init(&io_table);

    return VFS_ENTRY_MODULE(&io_table, tag, strlen(tag) - 1, 0, 0, 0, vfs_mount);
}

int sys_mount(const char *source, const char *target, const char *filesystemtype, unsigned long mountflags, const void *data) {
}

int sys_umount(const char *target) {

    if (target == NULL) {
        return -1;
    }

    return VFS_ENTRY_MODULE(target, strlen(target) - 1, 0, 0, 0, 0, vfs_umount);
}

ssize_t sys_read(int fd, void *buf, size_t count) {

    return (ssize_t)VFS_ENTRY_MODULE(get_vfs_process(), fd, buf, count, 0, 0, vfs_read);
}

ssize_t sys_write(int fd, const void *buf, size_t count) {
    return (ssize_t)VFS_ENTRY_MODULE(get_vfs_process(), fd, buf, count, 0, 0, vfs_write);
}

int sys_open(const char *pathname, int flags, mode_t mode) {
    return VFS_ENTRY_MODULE(get_vfs_process(), pathname, strlen(pathname) - 1, flags, mode, 0, vfs_open);
}

int sys_close(int fd) {
    return VFS_ENTRY_MODULE(get_vfs_process(), fd, 0, 0, 0, 0, vfs_close);
}
