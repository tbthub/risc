
#include "vfs/vfs_interface.h"
#include "vfs/vfs_process.h"
#include "vfs/vfs_module.h"
#include "vfs/vfs_util.h"
#include "vfs/vfs_io.h"
#include <string.h>
#include <stdio.h>

void vfs_process_init(vfs_process_t *proc, uint8_t *root_path_s, uint8_t *work_path_s, uint16_t root_path_size_s, uint16_t work_path_size_s) {

    memset(proc, 0, sizeof(vfs_process_t));

    proc->root_path      = vfs_malloc(root_path_size_s);
    proc->work_path      = vfs_malloc(work_path_size_s);
    proc->rwlock         = vfs_rwlock_init();
    proc->root_path_size = root_path_size_s;
    proc->work_path_size = work_path_size_s;

    if (proc->root_path == NULL || proc->work_path == NULL || proc->rwlock == NULL) {
        goto cleanup;
    }

    for (int i = 0; i < VFS_FD_SIZE; i++) {
        proc->fd_list[i].rwlock = vfs_rwlock_init();

        if (proc->fd_list[i].rwlock == NULL) {
            goto cleanup;
        }
    }

    memcpy(proc->root_path, root_path_s, root_path_size_s);
    memcpy(proc->work_path, work_path_s, work_path_size_s);
    proc->root_path_size = root_path_size_s;
    proc->work_path_size = work_path_size_s;

    return;

cleanup:

    if (proc->root_path != NULL) {
        vfs_free(proc->root_path);
        proc->root_path      = NULL;
        proc->root_path_size = 0;
    }

    if (proc->work_path != NULL) {
        vfs_free(proc->work_path);
        proc->work_path      = NULL;
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

void vfs_process_deinit(vfs_process_t *proc) {

    for (int i = 0; i < VFS_FD_SIZE; i++) {
        vfs_rwlock_deinit(proc->fd_list[i].rwlock);
    }

    vfs_rwlock_deinit(proc->rwlock);
    vfs_free(proc->work_path);
    vfs_free(proc->root_path);
}

int vfs_alloc_fd(vfs_process_t *process) {

    int res_fd = -1;
    for (int i = 0; i < VFS_FD_SIZE; i++) {

        if (process->fd_list[i].ptr == 0) {
            res_fd = i;
            break;
        }
    }

    return res_fd;
}

void vfs_set_fd_context(vfs_process_t *process, int fd, uintptr_t *ptr, uint8_t *flag) {
    vfs_wlock_acquire(process->fd_list[fd].rwlock);
    *ptr  = process->fd_list[fd].ptr;
    *flag = process->fd_list[fd].flag;
}

void vfs_set_fd_context_done(vfs_process_t *process, int fd, uintptr_t ptr, uint8_t flag) {
    process->fd_list[fd].ptr  = ptr;
    process->fd_list[fd].flag = flag;
    vfs_wlock_release(process->fd_list[fd].rwlock);
}

void vfs_get_fd_context(vfs_process_t *process, int fd, uintptr_t *ptr, uint8_t *flag) {
    vfs_rlock_acquire(process->fd_list[fd].rwlock);
    *ptr  = process->fd_list[fd].ptr;
    *flag = process->fd_list[fd].flag;
}

void vfs_get_fd_context_done(vfs_process_t *process, int fd) {
    vfs_rlock_release(process->fd_list[fd].rwlock);
}

vfs_process_t *vfs_process_read() {
    vfs_process_t *proc = (vfs_process_t *)vfs_get_process();
    vfs_rlock_acquire(proc->rwlock);
    return proc;
}

void vfs_process_read_done() {
    vfs_process_t *proc = (vfs_process_t *)vfs_get_process();
    vfs_rlock_release(proc->rwlock);
}

vfs_process_t *vfs_process_write() {
    vfs_process_t *proc = (vfs_process_t *)vfs_get_process();
    vfs_wlock_acquire(proc->rwlock);
    return proc;
}

void vfs_process_write_done() {
    vfs_process_t *proc = (vfs_process_t *)vfs_get_process();
    vfs_wlock_release(proc->rwlock);
}

int vfs_copy_proc_to_new_proc(vfs_process_t *new_proc) {

    uintptr_t old_ctx_ptr;
    uint8_t old_flag;
    vfs_process_t *proc    = vfs_process_read();
    uint8_t *new_root_path = (uint8_t *)vfs_malloc(proc->root_path_size);
    uint8_t *new_work_path = (uint8_t *)vfs_malloc(proc->work_path_size);

    if (!new_root_path || !new_work_path) {
        goto failed;
    }

    for (int i = 0; i < VFS_FD_SIZE; i++) {

        vfs_get_fd_context(proc, i, &old_ctx_ptr, &old_flag);

        if (old_ctx_ptr == 0) {
            vfs_get_fd_context_done(proc, i);
            continue;
        }

        vfs_file_context_t* ctx = (vfs_file_context_t *)old_ctx_ptr;
        vfs_wlock_acquire(ctx->lock);
        ctx->ref++;
        vfs_wlock_release(ctx->lock);
        new_proc->fd_list[i].ptr  = old_ctx_ptr;
        new_proc->fd_list[i].flag = old_flag;
        vfs_get_fd_context_done(proc, i);
    }

    vfs_free(new_proc->root_path);
    vfs_free(new_proc->work_path);

    new_proc->root_path      = new_root_path;
    new_proc->work_path      = new_work_path;
    new_proc->root_path_size = proc->root_path_size;
    new_proc->work_path_size = proc->work_path_size;

    vfs_process_read_done();
    return 0;

failed:

    if (new_root_path) {
        vfs_free(new_root_path);
    }

    if (new_work_path) {
        vfs_free(new_work_path);
    }

    for (int i = 0; i < VFS_FD_SIZE; i++) {

        if (new_proc->fd_list[i].ptr != 0) {

            if (vfs_close_with_context((vfs_file_context_t *)new_proc->fd_list[i].ptr) < 0) {
                vfs_raise_err(6);
            }

            new_proc->fd_list[i].ptr  = 0;
            new_proc->fd_list[i].flag = 0;
        }
    }

    vfs_process_read_done();
    return -1;
}

vfs_file_context_t *vfs_open_with_context(vfs_process_t *proc, const char *path, int flag, int mode) {

    char *full_path = construct_full_path(proc->root_path, proc->work_path, proc->root_path_size, proc->work_path_size, path);
    if (!full_path) {
        return NULL;
    }

    char *mount_tag, *open_path;
    if (parse_mount_tag(full_path, &mount_tag, &open_path) != 0) {
        vfs_free(full_path);
        return NULL;
    }

    size_t mountTagSize     = strlen(mount_tag) + 1;
    vfs_io_t *mount_io      = vfs_data_global_get((uint8_t *)mount_tag, mountTagSize);
    vfs_file_context_t *ctx = vfs_malloc(sizeof(vfs_file_context_t));
    void *ctx_lock          = vfs_rwlock_init();

    if (!mount_io || !ctx || !ctx_lock) {
        goto cleanup;
    }

    ctx->mount_ptr = mount_io->mount_ptr;
    ctx->op        = mount_io;
    ctx->file_ptr  = NULL;
    ctx->flag      = flag;
    ctx->ref       = 1;
    ctx->lock      = ctx_lock;

    // 文件操作使用驱动内部锁
    if (mount_io->open(ctx, (uint8_t *)open_path, flag, mode) < 0) {
        goto cleanup;
    }

    vfs_free(mount_tag);
    vfs_free(open_path);
    vfs_free(full_path);

    return ctx;
cleanup:

    vfs_free(mount_tag);
    vfs_free(open_path);
    vfs_free(full_path);

    if (ctx_lock){
        vfs_rwlock_deinit(ctx_lock);
    }

    if (ctx){
        vfs_free(ctx);
    }

    if (mount_io){
        vfs_data_global_get_done((uint8_t *)mount_io->mount_ptr, mount_io->mountTagSize);
    }

    return NULL;
}

int vfs_close_with_context(vfs_file_context_t *ctx) {

    int res = 0;

    if (!ctx) {
        return -1;
    }

    void* ctx_lock = ctx->lock;
    vfs_wlock_acquire(ctx_lock);
    ctx->ref--;

    if (ctx->ref < 0) {
        vfs_raise_err(5);
    }

    if (ctx->ref == 0) {
        res = (int)ctx->op->close(ctx);
        vfs_data_global_get_done((uint8_t *)ctx->op->mountTag, ctx->op->mountTagSize);
        vfs_free(ctx);
        vfs_wlock_release(ctx_lock);
        vfs_rwlock_deinit(ctx_lock);
        return res;
    }

    vfs_wlock_release(ctx_lock);
    return res;
}

int vfs_read_with_context(vfs_file_context_t *ctx, const void *buf, size_t size) {
    if (!ctx || ((ctx->flag & 0b11) != VFS_O_RDWR && (ctx->flag & 0b11) != VFS_O_RDONLY) || ctx->op->read == NULL) {
        return -1;
    }


    vfs_rlock_acquire(ctx->lock);
    int res = (int)ctx->op->read(ctx, (uint8_t *)buf, size);
    vfs_rlock_release(ctx->lock);
    return res;
}

int vfs_write_with_context(vfs_file_context_t *ctx, void *buf, size_t size) {
    if (!ctx || ((ctx->flag & 0b11) != VFS_O_RDWR && (ctx->flag & 0b11) != VFS_O_WRONLY) || ctx->op->write == NULL) {
        return -1;
    }

    vfs_wlock_acquire(ctx->lock);
    int res = (int)ctx->op->write(ctx, (uint8_t *)buf, size);
    vfs_wlock_release(ctx->lock);
    return res;
}

int vfs_lseek_with_context(vfs_file_context_t *ctx, vfs_off_t offset, int whence) {
    if (!ctx || ctx->op->lseek == NULL) {
        return -1;
    }

    vfs_rlock_acquire(ctx->lock);
    int res = (int)ctx->op->lseek(ctx, offset, whence);
    vfs_rlock_release(ctx->lock);
    return res;
}

int vfs_dup_with_context(vfs_file_context_t *ctx){
    vfs_wlock_acquire(ctx->lock);
    ctx->ref++;
    vfs_wlock_release(ctx->lock);
    return 0;
}

int vfs_mkfs_add_file(vfs_io_t *table, const char *path, const void *buffer, size_t size) {
    vfs_file_context_t file_context;

    vfs_file_context_t *ctx = &file_context;
    ctx->mount_ptr          = table->mount_ptr;
    ctx->op                 = table;
    ctx->file_ptr           = NULL;
    ctx->flag               = VFS_O_CREAT | VFS_O_WRONLY;

    if (table->open(ctx, (uint8_t *)path, ctx->flag, 0) < 0) {
        return -1;
    }

    int res = (int)table->write(ctx, (uint8_t *)buffer, size);

    if (table->close(ctx) < 0) {
        vfs_raise_err(4);
    }

    return res;
}
