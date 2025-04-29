
#include "vfs/vfs_io.h"
#include "vfs/vfs_interface.h"
#include "vfs/vfs_util.h"
#include "vfs/vfs_process.h"
#include "vfs/vfs_module.h"
#include <string.h>
#include <stdio.h>

int vfs_open(const char *path, int flag, int mode) {

    uintptr_t _t;
    uint8_t _f;
    int fd;
    vfs_process_t *proc = vfs_process_read();

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
        goto cleanup;
    }

    vfs_file_context_t *ctx = vfs_open_with_context(proc, path, flag, mode);

    if (ctx == NULL) {
        goto cleanup;
    }

    // 设置FD上下文（自动管理写锁）
    vfs_set_fd_context_done(proc, fd, (uintptr_t)ctx, 0);
    vfs_process_read_done();
    return fd;

cleanup:
    vfs_set_fd_context_done(proc, fd, _t, _f);
    vfs_process_read_done();
    return -1;
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
    vfs_file_context_t *ctx = (vfs_file_context_t *)ctx_ptr;

    if (vfs_close_with_context(ctx) < 0) {
        vfs_set_fd_context_done(proc, fd, ctx_ptr, flag);
        vfs_process_read_done();
        return -1;
    }

    vfs_set_fd_context_done(proc, fd, 0, 0); // 清除并释放写锁
    vfs_process_read_done();
    return 0;
}

int vfs_read(int fd, const void *buf, size_t size) {

    if (fd >= VFS_FD_SIZE) {
        return -1;
    }

    vfs_process_t *proc = vfs_process_read(); // 进程级读锁

    uintptr_t ctx_ptr;
    uint8_t flag;

    // 获取FD读锁（阻塞其他线程的写操作）
    vfs_get_fd_context(proc, fd, &ctx_ptr, &flag);

    vfs_file_context_t *ctx = (vfs_file_context_t *)ctx_ptr;
    int ret                 = vfs_read_with_context(ctx, buf, size);

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
    int ret                 = vfs_write_with_context(ctx, buf, size);

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

    uintptr_t old_ctx_ptr;
    uint8_t old_flag;

    vfs_get_fd_context(proc, oldfd, &old_ctx_ptr, &old_flag);

    if (!old_ctx_ptr) {
        vfs_get_fd_context_done(proc, oldfd);
        vfs_process_write_done();
        return -1;
    }

    // 分配新FD
    int newfd;
    uintptr_t _t;
    uint8_t _f;

    while (1) {
        newfd = vfs_alloc_fd(proc);

        if (newfd < 0) {
            vfs_get_fd_context_done(proc, oldfd);
            vfs_process_write_done();
            return -1;
        }

        vfs_set_fd_context(proc, newfd, &_t, &_f); // 获取写锁
        if (_t == 0) {
            break;
        }

        vfs_set_fd_context_done(proc, newfd, _t, _f);
    }


    int res = vfs_dup_with_context((vfs_file_context_t *)old_ctx_ptr);
    
    vfs_set_fd_context_done(proc, newfd, old_ctx_ptr, old_flag);
    vfs_get_fd_context_done(proc, oldfd);
    vfs_process_write_done();

    if (res < 0){
        return -1;
    }
    else{
        return newfd;
    }
}

int vfs_dup2(int oldfd, int newfd) {

    if (oldfd >= VFS_FD_SIZE || newfd >= VFS_FD_SIZE) {
        return -1;
    }

    if (oldfd == newfd)
        return newfd;

    vfs_process_t *proc = vfs_process_write();

    uintptr_t old_ctx_ptr;
    uint8_t old_flag;

    vfs_get_fd_context(proc, oldfd, &old_ctx_ptr, &old_flag);

    if (!old_ctx_ptr) {
        vfs_get_fd_context_done(proc, oldfd);
        vfs_process_write_done();
        return -1;
    }


    uintptr_t _t;
    uint8_t _f;

    vfs_set_fd_context(proc, newfd, &_t, &_f); // 获取写锁

    if (_t != 0) {
        vfs_get_fd_context_done(proc, oldfd);
        vfs_process_write_done();
        return -1;
    }

    int res = vfs_dup_with_context((vfs_file_context_t *)old_ctx_ptr);

    vfs_set_fd_context_done(proc, newfd, old_ctx_ptr, old_flag);
    vfs_get_fd_context_done(proc, oldfd);
    vfs_process_write_done();
    
    if (res < 0) {
        return -1;
    } 
    else {
        return newfd;
    }
}
