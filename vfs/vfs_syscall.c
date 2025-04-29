#include "vfs/vfs_interface.h"
#include "vfs/vfs_io.h"
#include "vfs/vfs_module.h"
#include "std/stdint.h"
#include "core/proc.h"
#include "sys.h"

int do_open(const char *path, int flags, int mod) {
    return vfs_open(path, flags, mod);
}

int do_close(int fd) {
    return vfs_close(fd);
}

int do_read(int fd, const void *buf, size_t count) {
    return vfs_read(fd, buf, count);
}

int do_write(int fd, void *buf, size_t count) {
    return vfs_write(fd, buf, count);
}

int do_lseek(int fd, off_t offset, int whence) {
    return vfs_lseek(fd, offset, whence);
}

int k_copy_file(struct task_struct *ch) {
    vfs_copy_proc_to_new_proc(&ch->vfs_proc);
    return 0;
}

int k_file_init(struct task_struct *task) {

    vfs_process_t *proc = &task->vfs_proc;
    vfs_process_init(proc, (uint8_t *)"/", NULL, 1, 0);
    return 0;
}

int k_file_deinit(struct task_struct *task) {
    vfs_process_t *proc = &task->vfs_proc;
    vfs_process_deinit(proc);
    return 0;
}

void *k_file_mmap_init(int fd) {

    vfs_process_t *proc = vfs_process_read();

    uintptr_t ctx_ptr;
    uint8_t flag;

    vfs_get_fd_context(proc, fd, &ctx_ptr, &flag);

    if (!ctx_ptr) {
        vfs_process_read_done();
        return NULL;
    }

    int res = vfs_dup_with_context((vfs_file_context_t *)ctx_ptr);

    vfs_get_fd_context_done(proc, fd);
    vfs_process_read_done();

    if (res < 0){
        return NULL;
    }
    else{
        return (void *)ctx_ptr;
    }
}

void k_file_mmap_close(void *ctx) {
    vfs_file_context_t *ctx_ptr = (vfs_file_context_t *)ctx;
    vfs_close_with_context(ctx_ptr);
    vfs_free(ctx);
}

void *k_file_mmap_dup(void *ctx) {
    vfs_file_context_t *vfs_ctx = (vfs_file_context_t *)ctx;
    vfs_wlock_acquire(vfs_ctx->lock);
    vfs_ctx->ref++;
    vfs_wlock_release(vfs_ctx->lock);
    return ctx;
}

int k_file_mmap_read(void *ctx, const void *buf, size_t size) {
    return vfs_read_with_context((vfs_file_context_t *)ctx, buf, size);
}

int k_file_mmap_lseek(void *ctx, off_t offset, int whence) {
    return vfs_lseek_with_context((vfs_file_context_t *)ctx, offset, whence);
}

int k_file_read_no_off(int fd, vfs_off_t off, void *buf, size_t size) {
    int res;
    int cur_offset = vfs_lseek(fd, 0, VFS_SEEK_CUR);
    assert(cur_offset >= 0, "k_file_read_no_off 1");

    res = vfs_lseek(fd, off, VFS_SEEK_SET);
    if (res < 0){
        goto end;
    }

    res = vfs_read(fd, buf, size);

end:
    assert(vfs_lseek(fd, cur_offset, VFS_SEEK_SET) >= 0, "k_file_read_no_off 2");

    return res;
}
