#include "vfs/vfs_interface.h"
#include "vfs/vfs_io.h"
#include "vfs/vfs_module.h"
#include "std/stdint.h"
#include "core/proc.h"
#include "sys.h"


int do_open(const char *path, int flags, int mod){
    return vfs_open(path, flags, mod);
}

int do_close(int fd){
    return vfs_close(fd);
}

int do_read(int fd, const void *buf, size_t count){
    return vfs_read(fd, buf, count);
}

int do_write(int fd, void *buf, size_t count) {
    return vfs_write(fd, buf, count);
}

int do_lseek(int fd, off_t offset, int whence){
    return vfs_lseek(fd, offset, whence);
}

int k_copy_file(struct task_struct *ch) {
    vfs_copy_filectx_to_new_proc(&ch->vfs_proc);
    return 0;
}

int k_file_init(struct task_struct *task) {

    vfs_process_t *proc = &task->vfs_proc;
    vfs_process_init(proc, proc->root_path, proc->work_path, proc->root_path_size, proc->work_path_size);
    return 0;

}

int k_file_deinit(struct task_struct *task) {
    vfs_process_t *proc = &task->vfs_proc;
    vfs_process_deinit(proc);
    return 0;
}
