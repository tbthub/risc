#include "vfs/vfs_io.h"
#include "vfs/vfs_interface.h"
#include "vfs/vfs_util.h"
#include "vfs/vfs_process.h"

#include <string.h>

int vfs_chroot(const char *path) {
    vfs_process_t *proc = vfs_process_write();
    char *new_root      = construct_full_path(proc->root_path, proc->work_path, proc->root_path_size, proc->work_path_size, path);
    if (!new_root) {
        vfs_process_write_done();
        return -1;
    }

    size_t len = strlen(new_root);
    if (len > UINT16_MAX) {
        vfs_free(new_root);
        vfs_process_write_done();
        return -1;
    }

    proc->root_path      = new_root;
    proc->root_path_size = len;
    vfs_process_write_done();
    return 0;
}

char *vfs_getcwd(char *buf, size_t size) {
    vfs_process_t *proc = vfs_process_read();
    size_t len          = proc->work_path_size;
    if (len > size) {
        vfs_process_read_done();
        return NULL;
    }
    memcpy(buf, proc->work_path, len);
    vfs_process_read_done();
    return buf;
}

int vfs_chdir(const char *path) {
    vfs_process_t *proc = vfs_process_write();
    char *new_work      = construct_full_path("", proc->work_path, 0, proc->work_path_size, path);
    if (!new_work) {
        vfs_process_write_done();
        return -1;
    }

    size_t len = strlen(new_work);
    if (len > UINT16_MAX) {
        vfs_free(new_work);
        vfs_process_write_done();
        return -1;
    }

    proc->work_path      = new_work;
    proc->work_path_size = len;
    vfs_process_write_done();
    return 0;
}
