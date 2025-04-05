
#include "vfs/vfs_io.h"
#include "vfs/vfs_interface.h"
#include "vfs/vfs_util.h"
#include "vfs/vfs_process.h"
#include <string.h>

int vfs_open(const char *path, int flag, int mode) {

    vfs_process_t *proc = vfs_process_read();

    char *full_path = construct_full_path(proc->root_path, proc->work_path, proc->root_path_size, proc->work_path_size, path);
    if (!full_path) {
        vfs_process_read_done();
        return -1;
    }

    char *mount_tag, *open_path;
    if (parse_mount_tag(full_path, &mount_tag, &open_path) != 0) {
        vfs_free(full_path);
        vfs_process_read_done();
        return -1;
    }

    size_t mountTagSize = strlen(mount_tag) + 1;

    vfs_io_t *mount_io = vfs_data_global_get((uint8_t *)mount_tag, mountTagSize);

    if (!mount_io) {
        vfs_free(mount_tag);
        vfs_free(open_path);
        vfs_free(full_path);
        vfs_process_read_done();
        return -1;
    }

    uintptr_t _t;
    uint8_t _f;
    int fd;

    while (1) {
        fd = vfs_alloc_fd(proc);

        if (fd < 0) {
            break;
        }

        vfs_set_fd_context(proc, fd, &_t, &_f); // 获取写锁

        if (_t == 0) {
            break;
        }
        vfs_set_fd_context_done(proc, fd, _t, _f);
    }

    if (fd < 0) {
        vfs_data_global_get_done((uint8_t *)mount_tag, mountTagSize);
        vfs_free(mount_tag);
        vfs_free(open_path);
        vfs_free(full_path);
        vfs_set_fd_context_done(proc, fd, _t, _f);
        vfs_process_read_done();
        return -1;
    }

    vfs_file_context_t *ctx = vfs_malloc(sizeof(vfs_file_context_t));
    ctx->mount_ptr          = mount_io->mount_ptr;
    ctx->op                 = mount_io;
    ctx->file_ptr           = NULL;
    ctx->flag               = flag;

    // 文件操作使用驱动内部锁
    if (mount_io->open(ctx, (uint8_t *)open_path, flag, mode) < 0) {
        vfs_free(ctx);
        vfs_data_global_get_done((uint8_t *)ctx->op->mountTag, ctx->op->mountTagSize);
        vfs_free(mount_tag);
        vfs_free(open_path);
        vfs_free(full_path);
        vfs_process_read_done();
        return -1;
    }

    // 设置FD上下文（自动管理写锁）
    vfs_set_fd_context_done(proc, fd, (uintptr_t)ctx, 0);
    vfs_free(mount_tag);
    vfs_free(open_path);
    vfs_free(full_path);
    vfs_process_read_done(); // 释放进程写锁
    return fd;
}

int vfs_close(int fd) {

    if (fd >= VFS_FD_SIZE) {
        return -1;
    }

    vfs_process_t *proc = vfs_process_read();

    // 获取FD写锁
    uintptr_t ctx_ptr;
    uint8_t flag;

    vfs_set_fd_context(proc, fd, &ctx_ptr, &flag);
    if (!ctx_ptr) {
        vfs_set_fd_context_done(proc, fd, ctx_ptr, flag);
        vfs_process_read_done();
        return -1;
    }

    vfs_file_context_t *ctx = (vfs_file_context_t *)ctx_ptr;
    int8_t ret              = ctx->op->close(ctx);

    if (ret >= 0) {
        vfs_data_global_get_done((uint8_t *)ctx->op->mountTag, ctx->op->mountTagSize);
        vfs_free(ctx);
    }
    vfs_set_fd_context_done(proc, fd, 0, 0); // 清除并释放写锁
    vfs_process_read_done();
    return (int)ret;
}

int vfs_read(int fd, void *buf, size_t size) {

    if (fd >= VFS_FD_SIZE) {
        return -1;
    }

    vfs_process_t *proc = vfs_process_read(); // 进程级读锁

    uintptr_t ctx_ptr;
    uint8_t flag;

    // 获取FD读锁（阻塞其他线程的写操作）
    vfs_get_fd_context(proc, fd, &ctx_ptr, &flag);

    vfs_file_context_t *ctx = (vfs_file_context_t *)ctx_ptr;
    if (!ctx_ptr || ((ctx->flag & 0b11) != VFS_O_RDWR && (ctx->flag & 0b11) != VFS_O_RDONLY)) {
        vfs_get_fd_context_done(proc, fd);
        vfs_process_read_done();
        return -1;
    }

    int32_t ret = ctx->op->read(ctx, (uint8_t *)buf, size);
    vfs_get_fd_context_done(proc, fd);
    vfs_process_read_done();
    return (int)ret;
}

int vfs_write(int fd, void *buf, size_t size) {

    if (fd >= VFS_FD_SIZE) {
        return -1;
    }

    vfs_process_t *proc = vfs_process_read();

    uintptr_t ctx_ptr;
    uint8_t flag;

    // 获取FD写锁（独占访问）
    vfs_set_fd_context(proc, fd, &ctx_ptr, &flag);
    vfs_file_context_t *ctx = (vfs_file_context_t *)ctx_ptr;

    if (!ctx_ptr || ((ctx->flag & 0b11) != VFS_O_RDWR && (ctx->flag & 0b11) != VFS_O_WRONLY)) {
        vfs_set_fd_context_done(proc, fd, ctx_ptr, flag);
        vfs_process_read_done();
        return -1;
    }

    int32_t ret = ctx->op->write(ctx, (uint8_t *)buf, size);

    // 保持写锁直到操作完成
    vfs_set_fd_context_done(proc, fd, ctx_ptr, flag);
    vfs_process_read_done();
    return ret;
}

int vfs_lseek(int fd, vfs_off_t offset, int whence) {
    // 检查文件描述符有效性
    if (fd >= VFS_FD_SIZE) {
        return -1;
    }

    // 获取进程级读锁（不修改进程状态）
    vfs_process_t *proc = vfs_process_read();
    if (!proc) {
        return -1;
    }

    uintptr_t ctx_ptr;
    uint8_t flag;

    // 获取文件描述符的写锁（修改文件位置需独占访问）
    vfs_set_fd_context(proc, fd, &ctx_ptr, &flag);
    vfs_file_context_t *ctx = (vfs_file_context_t *)ctx_ptr;

    // 上下文有效性检查
    if (!ctx || !ctx->op->lseek) {
        vfs_set_fd_context_done(proc, fd, ctx_ptr, flag);
        vfs_process_read_done();
        return -1;
    }

    // 调用驱动层lseek实现
    int32_t ret = ctx->op->lseek(ctx, (int32_t)offset, whence);

    // 释放锁并返回结果
    vfs_set_fd_context_done(proc, fd, ctx_ptr, flag);
    vfs_process_read_done();
    return (int)ret;
}

int vfs_dup(int oldfd) {

    if (oldfd >= VFS_FD_SIZE) {
        return -1;
    }

    vfs_process_t *proc = vfs_process_write();

    // 分配新FD
    int newfd;
    uintptr_t _t;
    uint8_t _f;

    while (1) {
        newfd = vfs_alloc_fd(proc);

        if (newfd < 0) {
            vfs_process_write_done();
            return -1;
        }

        vfs_set_fd_context(proc, newfd, &_t, &_f); // 获取写锁
        if (_t == 0) {
            break;
        }

        vfs_set_fd_context_done(proc, newfd, _t, _f);
    }

    uintptr_t old_ctx_ptr;
    uint8_t old_flag;

    vfs_get_fd_context(proc, oldfd, &old_ctx_ptr, &old_flag);

    if (!old_ctx_ptr) {
        vfs_get_fd_context_done(proc, oldfd);
        vfs_set_fd_context_done(proc, newfd, _t, _f);
        vfs_process_write_done();
        return -1;
    }

    vfs_file_context_t *old_ctx = (vfs_file_context_t *)old_ctx_ptr;
    vfs_io_t *io                = old_ctx->op;

    vfs_file_context_t *new_ctx = vfs_malloc(sizeof(vfs_file_context_t));

    if (new_ctx == NULL) {
        vfs_get_fd_context_done(proc, oldfd);
        vfs_set_fd_context_done(proc, newfd, _t, _f);
        vfs_process_write_done();
        return -1;
    }

    memcpy(new_ctx, old_ctx, sizeof(vfs_file_context_t));
    new_ctx->file_ptr = NULL;

    if (io->dup2 == NULL || io->dup2(old_ctx, new_ctx) < 0) {
        vfs_free(new_ctx);
        vfs_get_fd_context_done(proc, oldfd);
        vfs_set_fd_context_done(proc, newfd, _t, _f);
        vfs_process_write_done();
        return -1;
    }

    vfs_get_fd_context_done(proc, oldfd);
    vfs_set_fd_context_done(proc, newfd, (uintptr_t)new_ctx, old_flag);
    vfs_data_global_get((uint8_t *)new_ctx->op->mountTag, new_ctx->op->mountTagSize); // 增加引用计数
    vfs_process_write_done();
    return newfd;
}

int vfs_dup2(int oldfd, int newfd) {

    if (oldfd >= VFS_FD_SIZE || newfd >= VFS_FD_SIZE) {
        return -1;
    }

    if (oldfd == newfd)
        return newfd;

    vfs_process_t *proc = vfs_process_write();
    uintptr_t target_ctx;
    uint8_t target_flag;

    // 获取目标FD写锁
    vfs_set_fd_context(proc, newfd, &target_ctx, &target_flag);

    // 关闭原有文件
    if (target_ctx) {
        vfs_file_context_t *ctx = (vfs_file_context_t *)target_ctx;
        ctx->op->close(ctx);
        vfs_free(ctx);
    }

    // 获取原FD读锁
    uintptr_t old_ctx_ptr;
    uint8_t old_flag;
    vfs_get_fd_context(proc, oldfd, &old_ctx_ptr, &old_flag);
    if (!old_ctx_ptr) {
        vfs_set_fd_context_done(proc, newfd, 0, 0);
        vfs_get_fd_context_done(proc, oldfd);
        vfs_process_write_done();
        return -1;
    }

    // 复制上下文
    vfs_file_context_t *old_ctx = (vfs_file_context_t *)old_ctx_ptr;
    vfs_io_t *io                = old_ctx->op;
    vfs_file_context_t *new_ctx = vfs_malloc(sizeof(vfs_file_context_t));
    if (new_ctx == NULL) {
        vfs_set_fd_context_done(proc, newfd, 0, 0);
        vfs_get_fd_context_done(proc, oldfd);
        vfs_process_write_done();
        return -1;
    }

    memcpy(new_ctx, old_ctx, sizeof(vfs_file_context_t));
    new_ctx->file_ptr = NULL;

    // 当操作不存在或失败时返回
    if (io->dup2 == NULL || io->dup2(old_ctx, new_ctx) < 0) {
        vfs_free(new_ctx);
        vfs_set_fd_context_done(proc, newfd, 0, 0);
        vfs_get_fd_context_done(proc, oldfd);
        vfs_process_write_done();
        return -1;
    }

    // 更新目标FD
    vfs_set_fd_context_done(proc, newfd, (uintptr_t)new_ctx, old_flag);
    vfs_get_fd_context_done(proc, oldfd);
    vfs_data_global_get((uint8_t *)new_ctx->op->mountTag, new_ctx->op->mountTagSize); // 增加引用计数
    vfs_process_write_done();
    return newfd;
}
