
#include "fs/vfs/vfs_io.h"
#include "fs/vfs/vfs_interface.h"
#include "fs/vfs/vfs_module.h"
#include <string.h>

int vfs_mount(vfs_io_t *mountIo, void* arg) {

    size_t mountTagSize   = strlen(mountIo->mountTag) + 1;
    mountIo->mountTagSize = mountTagSize;

    // 调用驱动的mount函数
    void *mount_ptr = mountIo->mount(arg);
    if (mount_ptr == NULL) {
        return -1;
    }

    mountIo->mount_ptr = mount_ptr;

    // 将mountTag和mountIo存入全局表
    if (vfs_data_global_put((uint8_t *)mountIo->mountTag, mountTagSize, mountIo, sizeof(vfs_io_t)) < 0) {
        mountIo->umount(mount_ptr);
        return -1;
    }

    return 0;
}

int vfs_umount(const char *mountTag) {

    size_t mountTagSize = strlen(mountTag) + 1;
    vfs_io_t *mountIo   = (vfs_io_t *)vfs_data_global_del_start((uint8_t *)mountTag, mountTagSize);
    if (mountIo == NULL) {
        return -1;
    }

    // 调用驱动的umount函数
    if (mountIo->umount(mountIo->mount_ptr) < 0) {
        vfs_data_global_del_rollback((uint8_t *)mountTag, mountTagSize);
        return -1;
    }

    // 从全局表中删除
    vfs_data_global_del_end((uint8_t *)mountTag, mountTagSize);
    return 0;
}
