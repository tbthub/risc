
#ifdef __cplusplus
extern "C" {
#endif

#ifndef _VFS_INTERFACE_H
#define _VFS_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

void *vfs_malloc(size_t size);
void vfs_free(void *ptr);

void vfs_raise_err(uint16_t error);

int8_t vfs_data_global_put(uint8_t *key, size_t key_size, void *value, size_t value_size); // 若存在或处于删除状态则返回-1
void *vfs_data_global_get(uint8_t *key, size_t key_size);                                  // 获取，若处于删除状态中则返回NULL
void vfs_data_global_get_done(uint8_t *key, size_t key_size);
void *vfs_data_global_del_start(uint8_t *key, size_t key_size);         // ，若是，则成功返回其键值
void vfs_data_global_del_rollback(uint8_t *key, size_t key_size);       // 回滚回删除开始前的状态
void vfs_data_global_del_end(uint8_t *key, size_t key_size);            // 确定删除
void vfs_data_global_iter(bool (*callback)(uint8_t *, size_t, void *)); // 遍历，且会自动增减引用计数。当callback返回false，终止遍历

// 读写锁
void *vfs_rwlock_init();
void vfs_wlock_acquire(void *lock);
void vfs_wlock_release(void *lock);
void vfs_rlock_acquire(void *lock);
void vfs_rlock_release(void *lock);
void vfs_rwlock_deinit(void *lock);

void *vfs_get_process();

#endif

#ifdef __cplusplus
}
#endif
