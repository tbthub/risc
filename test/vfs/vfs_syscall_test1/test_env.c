#include <pthread.h>
#include "std/stdbool.h"
#include "std/stdint.h"
#include <stdlib.h>
#include "lib/string.h"
#include "vfs/vfs_interface.h"
#include "vfs/vfs_io.h"
#include "vfs/vfs_process.h"

/* 内存管理接口 */
int mem_ref = 0;

void *vfs_malloc(size_t size) {
    mem_ref++;
    return malloc(size);
}

void vfs_free(void *ptr) {
    mem_ref--;
    free(ptr);
}

void vfs_raise_err(uint16_t error) {
    // 错误处理实现（可根据需要扩展）
}

/* 全局数据存储实现 */
#define HASH_SIZE 128
typedef struct GlobalEntry {
    uint8_t *key;
    size_t key_size;
    void *value;
    size_t value_size;
    int ref_count;
    bool deleting;
    struct GlobalEntry *next;
} GlobalEntry;

static GlobalEntry *global_table[HASH_SIZE];
static pthread_rwlock_t global_lock = PTHREAD_RWLOCK_INITIALIZER;

static unsigned hash(const uint8_t *key, size_t key_size) {
    unsigned hash = 5381;
    for (size_t i = 0; i < key_size; i++) {
        hash = ((hash << 5) + hash) + key[i];
    }
    return hash % HASH_SIZE;
}

int8_t vfs_data_global_put(uint8_t *key, size_t key_size, void *value, size_t value_size) {
    pthread_rwlock_wrlock(&global_lock);

    unsigned idx        = hash(key, key_size);
    GlobalEntry **entry = &global_table[idx];
    while (*entry) {
        if ((*entry)->key_size == key_size &&
            memcmp((*entry)->key, key, key_size) == 0) {
            pthread_rwlock_unlock(&global_lock);
            return -1;
        }
        entry = &(*entry)->next;
    }

    GlobalEntry *new_entry = vfs_malloc(sizeof(GlobalEntry));
    new_entry->key         = vfs_malloc(key_size);
    memcpy(new_entry->key, key, key_size);
    new_entry->key_size = key_size;
    new_entry->value    = vfs_malloc(value_size);
    memcpy(new_entry->value, value, value_size);
    new_entry->value_size = value_size;
    new_entry->ref_count  = 0;
    new_entry->deleting   = false;
    new_entry->next       = NULL;

    *entry = new_entry;
    pthread_rwlock_unlock(&global_lock);
    return 0;
}

void *vfs_data_global_get(uint8_t *key, size_t key_size) {
    pthread_rwlock_rdlock(&global_lock);

    unsigned idx       = hash(key, key_size);
    GlobalEntry *entry = global_table[idx];
    while (entry) {
        if (entry->key_size == key_size &&
            memcmp(entry->key, key, key_size) == 0) {
            if (entry->deleting) {
                pthread_rwlock_unlock(&global_lock);
                return NULL;
            }
            __sync_fetch_and_add(&entry->ref_count, 1);
            pthread_rwlock_unlock(&global_lock);
            return entry->value;
        }
        entry = entry->next;
    }

    pthread_rwlock_unlock(&global_lock);
    return NULL;
}

void vfs_data_global_get_done(uint8_t *key, size_t key_size) {
    pthread_rwlock_rdlock(&global_lock);

    unsigned idx       = hash(key, key_size);
    GlobalEntry *entry = global_table[idx];
    while (entry) {
        if (entry->key_size == key_size &&
            memcmp(entry->key, key, key_size) == 0) {
            __sync_fetch_and_sub(&entry->ref_count, 1);
            break;
        }
        entry = entry->next;
    }

    pthread_rwlock_unlock(&global_lock);
}

void *vfs_data_global_del_start(uint8_t *key, size_t key_size) {
    pthread_rwlock_wrlock(&global_lock);

    unsigned idx        = hash(key, key_size);
    GlobalEntry **entry = &global_table[idx];
    while (*entry) {
        if ((*entry)->key_size == key_size &&
            memcmp((*entry)->key, key, key_size) == 0) {
            if ((*entry)->ref_count == 0 && !(*entry)->deleting) {
                (*entry)->deleting = true;
                vfs_io_t *value    = (*entry)->value;
                pthread_rwlock_unlock(&global_lock);
                return (void *)value;
            }
            break;
        }
        entry = &(*entry)->next;
    }

    pthread_rwlock_unlock(&global_lock);
    return NULL;
}

void vfs_data_global_del_rollback(uint8_t *key, size_t key_size) {
    pthread_rwlock_wrlock(&global_lock);

    unsigned idx       = hash(key, key_size);
    GlobalEntry *entry = global_table[idx];
    while (entry) {
        if (entry->key_size == key_size &&
            memcmp(entry->key, key, key_size) == 0) {
            entry->deleting = false;
            break;
        }
        entry = entry->next;
    }

    pthread_rwlock_unlock(&global_lock);
}

void vfs_data_global_del_end(uint8_t *key, size_t key_size) {
    pthread_rwlock_wrlock(&global_lock);

    unsigned idx         = hash(key, key_size);
    GlobalEntry **prev   = &global_table[idx];
    GlobalEntry *current = *prev;
    while (current) {
        if (current->key_size == key_size &&
            memcmp(current->key, key, key_size) == 0 &&
            current->deleting && current->ref_count == 0) {
            *prev = current->next;
            vfs_free(current->key);
            vfs_free(current->value);
            vfs_free(current);
            break;
        }
        prev    = &current->next;
        current = current->next;
    }

    pthread_rwlock_unlock(&global_lock);
}

void vfs_data_global_iter(bool (*callback)(uint8_t *, size_t, void *)) {
    pthread_rwlock_rdlock(&global_lock);

    for (int i = 0; i < HASH_SIZE; i++) {
        GlobalEntry *entry = global_table[i];
        while (entry) {
            if (!entry->deleting) {
                __sync_fetch_and_add(&entry->ref_count, 1);
                bool cont = callback(entry->key, entry->key_size, entry->value);
                __sync_fetch_and_sub(&entry->ref_count, 1);
                if (!cont) {
                    pthread_rwlock_unlock(&global_lock);
                    return;
                }
            }
            entry = entry->next;
        }
    }

    pthread_rwlock_unlock(&global_lock);
}

/* 读写锁实现 */
typedef struct {
    pthread_rwlock_t rwlock;
} VFSRWLock;

void *vfs_rwlock_init() {
    VFSRWLock *lock = vfs_malloc(sizeof(VFSRWLock));
    pthread_rwlock_init(&lock->rwlock, NULL);
    return lock;
}

void vfs_wlock_acquire(void *lock) {
    pthread_rwlock_wrlock(&((VFSRWLock *)lock)->rwlock);
}

void vfs_wlock_release(void *lock) {
    pthread_rwlock_unlock(&((VFSRWLock *)lock)->rwlock);
}

void vfs_rlock_acquire(void *lock) {
    pthread_rwlock_rdlock(&((VFSRWLock *)lock)->rwlock);
}

void vfs_rlock_release(void *lock) {
    pthread_rwlock_unlock(&((VFSRWLock *)lock)->rwlock);
}

void vfs_rwlock_deinit(void *lock) {
    pthread_rwlock_destroy(&((VFSRWLock *)lock)->rwlock);
    vfs_free(lock);
}

/* 进程上下文管理 */
static pthread_key_t process_key;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;

static void create_key() {
    pthread_key_create(&process_key, vfs_free);
}

static vfs_process_t process_desc;
static int process_desc_isinit = 0;

void *vfs_get_process() {
    return (void *)&process_desc;
}
