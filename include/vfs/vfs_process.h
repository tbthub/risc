#ifdef __cplusplus
extern "C" {
#endif

#ifndef _VFS_PROCESS_H
#define _VFS_PROCESS_H

#include "std/stdint.h"

#define VFS_FD_SIZE 32
#define VFS_MAX_PATH_SIZE 256

typedef struct vfs_fd_context_t {
    uintptr_t ptr;
    uint8_t flag;
    void *rwlock;
} vfs_fd_context_t;

typedef struct vfs_process_t {
    uint8_t *root_path;      // 不带\0
    uint16_t root_path_size; // 不算入\0
    uint8_t *work_path;
    uint16_t work_path_size;
    vfs_fd_context_t fd_list[VFS_FD_SIZE];
    void *rwlock;

} vfs_process_t;

int vfs_alloc_fd(vfs_process_t *process);
void vfs_set_fd_context(vfs_process_t *process, int fd, uintptr_t *ptr, uint8_t *flag);    // 等价于获取写锁并执行vfs_get_fd_context
void vfs_set_fd_context_done(vfs_process_t *process, int fd, uintptr_t ptr, uint8_t flag); // 将fd与对应的文件描述符相映射并释放写锁
void vfs_get_fd_context(vfs_process_t *process, int fd, uintptr_t *ptr, uint8_t *flag);    // 获取fd对应的文件描述符并获取读锁
void vfs_get_fd_context_done(vfs_process_t *process, int fd);                              // 释放fd对应的文件描述符的读锁

vfs_process_t *vfs_process_read(); // 获取当前进程的vfs状态描述符与读锁
void vfs_process_read_done();
vfs_process_t *vfs_process_write(); // 获取当前进程的vfs状态描述符与写锁
void vfs_process_write_done();      //

void vfs_process_init(uint8_t *root_path_s, uint8_t *work_path_s, uint16_t root_path_size_s, uint16_t work_path_size_s);

#endif

#ifdef __cplusplus
}
#endif
