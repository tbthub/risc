
#include "vfs_interface.h"
#include "vfs_process.h"
#include <string.h>

void vfs_process_init(uint8_t *root_path_s, uint8_t *work_path_s, uint16_t root_path_size_s, uint16_t work_path_size_s) {

    vfs_process_t *proc = (vfs_process_t *)vfs_get_process();
    memset(proc, 0, sizeof(vfs_process_t));

    proc->root_path = vfs_malloc(root_path_size_s);
    proc->work_path = vfs_malloc(work_path_size_s);
    proc->rwlock = vfs_rwlock_init();
    proc->root_path_size = root_path_size_s;
    proc->work_path_size = work_path_size_s;

    if (proc->root_path == NULL || proc->work_path == NULL || proc->rwlock == NULL){
        goto cleanup;
    }

    for (int i = 0;i < VFS_FD_SIZE;i++){
        proc->fd_list[i].rwlock = vfs_rwlock_init();

        if (proc->fd_list[i].rwlock == NULL){
            goto cleanup;
        }

    }

    memcpy(proc->root_path, root_path_s, root_path_size_s);
    memcpy(proc->work_path, work_path_s, work_path_size_s);
    proc->root_path_size = root_path_size_s;
    proc->work_path_size = work_path_size_s;

    return;

cleanup:

    if (proc->root_path != NULL){
        vfs_free(proc->root_path);
        proc->root_path = NULL;
        proc->root_path_size = 0;
    }

    if (proc->work_path != NULL){
        vfs_free(proc->work_path);
        proc->work_path = NULL;
        proc->work_path_size = 0;
    }

    if (proc->rwlock != NULL) {
        vfs_rwlock_deinit(proc->rwlock);
        proc->rwlock = NULL;
    }

    for (int i = 0; i < VFS_FD_SIZE; i++) {
        if (proc->fd_list[i].rwlock != NULL) {
            vfs_rwlock_deinit(proc->fd_list[i].rwlock);
            proc->fd_list[i].rwlock = NULL;
        }
    }

    return;
}

int vfs_alloc_fd(vfs_process_t *process){

    int res_fd = -1;
    for (int i = 0; i < VFS_FD_SIZE; i++){

        if (process->fd_list[i].ptr == 0){
            res_fd = i;
            break;
        }
    }

    return res_fd;
}

void vfs_set_fd_context(vfs_process_t *process, int fd, uintptr_t *ptr, uint8_t *flag){
    vfs_wlock_acquire(process->fd_list[fd].rwlock);
    *ptr = process->fd_list[fd].ptr;
    *flag = process->fd_list[fd].flag;
}

void vfs_set_fd_context_done(vfs_process_t *process, int fd, uintptr_t ptr, uint8_t flag){
    process->fd_list[fd].ptr = ptr;
    process->fd_list[fd].flag = flag;
    vfs_wlock_release(process->fd_list[fd].rwlock);
}

void vfs_get_fd_context(vfs_process_t *process, int fd, uintptr_t *ptr, uint8_t *flag){
    vfs_rlock_acquire(process->fd_list[fd].rwlock);
    *ptr  = process->fd_list[fd].ptr;
    *flag = process->fd_list[fd].flag;
}

void vfs_get_fd_context_done(vfs_process_t *process, int fd){
    vfs_rlock_release(process->fd_list[fd].rwlock);
}

vfs_process_t *vfs_process_read(){
    vfs_process_t *proc = (vfs_process_t *)vfs_get_process();
    vfs_rlock_acquire(proc->rwlock);
    return proc;
}

void vfs_process_read_done(){
    vfs_process_t *proc = (vfs_process_t *)vfs_get_process();
    vfs_rlock_release(proc->rwlock);
}

vfs_process_t *vfs_process_write(){
    vfs_process_t *proc = (vfs_process_t *)vfs_get_process();
    vfs_wlock_acquire(proc->rwlock);
    return proc;
}

void vfs_process_write_done(){
    vfs_process_t *proc = (vfs_process_t *)vfs_get_process();
    vfs_wlock_release(proc->rwlock);
}