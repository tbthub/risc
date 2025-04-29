#ifdef __cplusplus
extern "C" {
#endif

#ifndef _SIMFS_H
#define _SIMFS_H

#include "std/stdint.h"
#include "std/stddef.h"
#include "vfs/vfs_io.h"

// 文件节点结构体（链表形式）
typedef struct simfs_file_node {
    char *path;    // 文件路径
    uint8_t *data; // 文件数据
    size_t size;   // 文件大小
    size_t capacity;
    struct simfs_file_node *next; // 下一个节点
    void *lock;
} simfs_file_node_t;

// 文件系统结构体
typedef struct simfs_t {
    simfs_file_node_t *file_list; // 文件链表头
    void *lock;
} simfs_t;

// 文件上下文结构体
typedef struct simfs_file_t {   
    simfs_file_node_t *node; // 指向链表中的文件节点
    size_t pos;              // 当前读写位置
} simfs_file_t;

int8_t simfs_open(vfs_file_context_t *context, uint8_t *path, int flags, int mode);
int32_t simfs_read(vfs_file_context_t *context, uint8_t *buffer, size_t size);
int32_t simfs_write(vfs_file_context_t *context, uint8_t *buffer, size_t size);
int8_t simfs_close(vfs_file_context_t *context);
uint8_t *simfs_mount();
int8_t simfs_umount(uint8_t *sim_fs_ptr);
void simfs_io_table_init(vfs_io_t *table);

#endif
#ifdef __cplusplus
}
#endif
