#include "vfs/vfs_io.h"
#include "vfs/vfs_interface.h"
#include "fs/simfs/simfs.h"
#include "lib/string.h"
#include "std/stdio.h"

//--------------------------------------------------
// 内部工具函数：通过路径查找文件节点（假设调用者已持有适当的锁）
static simfs_file_node_t *find_file_node_locked(simfs_t *fs, const char *path) {
    simfs_file_node_t *current = fs->file_list;
    while (current) {
        if (strcmp(current->path, path) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// 内部工具函数：通过路径查找文件节点（使用读锁）
static simfs_file_node_t *find_file_node(simfs_t *fs, const char *path) {
    vfs_rlock_acquire(fs->lock);
    simfs_file_node_t *node = find_file_node_locked(fs, path);
    vfs_rlock_release(fs->lock);
    return node;
}

//--------------------------------------------------
// 打开文件（自动创建新文件）
int8_t simfs_open(vfs_file_context_t *context, uint8_t *path, int flags, int mode) {
    simfs_t *fs          = (simfs_t *)context->mount_ptr;
    const char *path_str = (const char *)path;


    // 查找现有文件
    simfs_file_node_t *node = find_file_node(fs, path_str);

    // 如果文件不存在且需要创建
    if (!node && (flags & VFS_O_CREAT)) {
        vfs_wlock_acquire(fs->lock);                // 获取写锁以修改文件列表
        node = find_file_node_locked(fs, path_str); // 再次检查是否存在
        if (!node) {
            node = (simfs_file_node_t *)vfs_malloc(sizeof(simfs_file_node_t));
            if (node == NULL) {
                vfs_wlock_release(fs->lock);
                goto init_new_node_failed;
            }

            node->lock = vfs_rwlock_init();
            if (node->lock == NULL) {
                vfs_wlock_release(fs->lock);
                vfs_free(node);
                goto init_new_node_failed;
            }

            node->path = (char *)vfs_malloc(strlen(path_str) + 1);
            if (node->path == NULL) {
                vfs_rwlock_deinit(node->lock);
                vfs_wlock_release(fs->lock);
                vfs_free(node);
                goto init_new_node_failed;
            }

            strcpy(node->path, path_str);
            node->data     = vfs_malloc(16);
            node->size     = 0;
            node->capacity = 16;

            // 插入链表头部
            node->next    = fs->file_list;
            fs->file_list = node;
        }
        vfs_wlock_release(fs->lock);
    }

    if (!node)
        return -1; // 文件不存在且未创建

    // 创建文件上下文
    simfs_file_t *file_ctx = (simfs_file_t *)vfs_malloc(sizeof(simfs_file_t));
    if (!file_ctx)
        return -1;

    file_ctx->node    = node;
    context->file_ptr = (uint8_t *)file_ctx;

    // 根据打开模式重置位置
    if (flags & VFS_O_APPEND) {
        file_ctx->pos = node->size;
    } else {
        file_ctx->pos = 0;
    }

    return 0; // 成功

init_new_node_failed:
    return -1;
}

//--------------------------------------------------
// 读取文件
int32_t simfs_read(vfs_file_context_t *context, uint8_t *buffer, size_t size) {
    simfs_file_t *file_ctx  = (simfs_file_t *)context->file_ptr;
    simfs_file_node_t *node = file_ctx->node;

    // 计算可读字节数
    vfs_rlock_acquire(node->lock);
    size_t readable   = node->size - file_ctx->pos;
    size_t bytes_read = (size < readable) ? size : readable;

    if (bytes_read > 0) {
        memcpy(buffer, node->data + file_ctx->pos, bytes_read);
        file_ctx->pos += bytes_read;
    }
    vfs_rlock_release(node->lock);

    return bytes_read;
}

//--------------------------------------------------
// 写入文件（自动扩展数据空间）
int32_t simfs_write(vfs_file_context_t *context, uint8_t *buffer, size_t size) {
    simfs_file_t *file_ctx  = (simfs_file_t *)context->file_ptr;
    simfs_file_node_t *node = file_ctx->node;

    vfs_wlock_acquire(node->lock);

    // 空间不足时扩展
    size_t required_size = file_ctx->pos + size;
    if (required_size > node->capacity) {
        size_t new_capacity = required_size + (required_size >> 1);
        uint8_t *new_data   = vfs_malloc(new_capacity);
        if (!new_data) {
            vfs_wlock_release(node->lock);
            return -1;
        }

        memset(new_data + node->size, 0, new_capacity - node->size);
        memcpy(new_data, node->data, node->size);
        vfs_free(node->data);
        node->data     = new_data;
        node->capacity = new_capacity;
    }

    // 更新size到实际数据长度
    if (file_ctx->pos + size > node->size) {
        node->size = file_ctx->pos + size;
    }
    memcpy(node->data + file_ctx->pos, buffer, size);
    file_ctx->pos += size;
    vfs_wlock_release(node->lock);
    return size;
}

//--------------------------------------------------
// 关闭文件
int8_t simfs_close(vfs_file_context_t *context) {
    simfs_file_t *file_ctx = (simfs_file_t *)context->file_ptr;
    vfs_free(file_ctx);
    context->file_ptr = NULL;
    return 0;
}

//--------------------------------------------------
// 挂载文件系统
uint8_t *simfs_mount() {
    simfs_t *fs = (simfs_t *)vfs_malloc(sizeof(simfs_t));

    if (fs) {
        fs->file_list = NULL;
        fs->lock      = vfs_rwlock_init();
        if (!fs->lock) {
            vfs_free(fs);
            fs = NULL;
        }
    }
    return (uint8_t *)fs;
}

//--------------------------------------------------
// 卸载文件系统（清理所有资源）
int8_t simfs_umount(uint8_t *sim_fs_ptr) {
    simfs_t *fs                = (simfs_t *)sim_fs_ptr;
    simfs_file_node_t *current = fs->file_list;

    // 遍历释放所有文件节点
    while (current) {
        simfs_file_node_t *next = current->next;
        vfs_rwlock_deinit(current->lock);
        vfs_free(current->path);

        if (current->data) {
            vfs_free(current->data);
        }

        vfs_free(current);
        current = next;
    }

    vfs_rwlock_deinit(fs->lock);
    vfs_free(fs);
    return 0;
}

//--------------------------------------------------
// 调整文件指针位置
int32_t simfs_lseek(vfs_file_context_t *context, int32_t offset, int whence) {
    simfs_file_t *file_ctx = (simfs_file_t *)context->file_ptr;
    if (!file_ctx || !file_ctx->node) {
        return -1;
    }
    simfs_file_node_t *node = file_ctx->node;

    vfs_wlock_acquire(node->lock);

    int64_t new_pos;

    switch (whence) {
    case VFS_SEEK_SET:
        new_pos = (int64_t)offset;
        break;
    case VFS_SEEK_CUR:
        new_pos = (int64_t)file_ctx->pos + (int64_t)offset;
        break;
    case VFS_SEEK_END:
        new_pos = (int64_t)node->size + (int64_t)offset;
        break;
    default:
        vfs_wlock_release(node->lock);
        return -1;
    }

    // 检查新位置是否合法
    if (new_pos < 0 || new_pos > 0x7fffffff) {
        vfs_wlock_release(node->lock);
        return -1;
    }

    file_ctx->pos = (size_t)new_pos;
    vfs_wlock_release(node->lock);

    return (int32_t)new_pos;
}

int8_t simfs_dup2(vfs_file_context_t *old_file, vfs_file_context_t *new_file) {
    simfs_file_t *file_ctx = (simfs_file_t *)old_file->file_ptr;
    new_file->file_ptr     = vfs_malloc(sizeof(simfs_file_t));
    if (new_file->file_ptr == NULL) {
        return -1;
    }

    memcpy(new_file->file_ptr, file_ctx, sizeof(simfs_file_t));
    return 0;
}

//--------------------------------------------------
// 文件系统表初始化
void simfs_io_table_init(vfs_io_t *table) {
    table->open   = simfs_open;
    table->read   = simfs_read;
    table->write  = simfs_write;
    table->close  = simfs_close;
    table->mount  = simfs_mount;
    table->umount = simfs_umount;
    table->lseek  = simfs_lseek;
    table->dup2   = simfs_dup2;
}
