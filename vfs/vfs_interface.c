#include "lib/hash.h"
#include "lib/string.h"
#include "mm/kmalloc.h"
#include "core/locks/rwlock.h"
#include "core/proc.h"
#include "vfs/vfs_interface.h"
#include <string.h>

typedef struct vfs_hash_t {
    uint8_t *key;
    size_t key_size;
    uint8_t *value;
    size_t value_size;
    hash_node_t node;
    int key_hash;
    int ref;
} vfs_hash_t;

extern struct hash_table hash_table_g;

int vfshash(uint8_t *key, size_t size) {
    unsigned long hash = 5381;

    for (size_t i = 0; i < size; i++) {
        hash = ((hash << 5) + hash) + key[i]; // hash * 33 + c
    }

    return (int)(hash & 0x7FFFFFFF);
}

int vfs_memcmp_n(void *buf1, size_t buf1_size, void *buf2, size_t buf2_size) {

    if (buf1_size != buf2_size) {
        return 1;
    }

    size_t min_size = buf1_size < buf2_size ? buf1_size : buf2_size;

    return memcmp(buf1, buf2, min_size);
}

void *vfs_malloc(size_t size) {
    return kmalloc((int)size, 0);
}

void vfs_free(void *ptr) {
    kfree(ptr);
}

void vfs_raise_err(uint16_t error) {
    panic("vfs_error: %d", error);
}

int8_t vfs_data_global_put(uint8_t *key, size_t key_size, void *value, size_t value_size) {

    int _key_hash = vfshash(key, key_size);
    vfs_hash_t *tmp;
    int find_it = -1;

    hash_for_each_entry(tmp, &hash_table_g, _key_hash, node) {

        if (vfs_memcmp_n(tmp->key, tmp->key_size, key, key_size) == 0) {
            find_it = 1;
            break;
        }
    }

    if (find_it == 1) {
        return -1;
    }

    vfs_hash_t *item = vfs_malloc(sizeof(vfs_hash_t));
    if (item == NULL) {
        goto cleanup;
    }

    item->key        = vfs_malloc(key_size);
    item->value      = vfs_malloc(value_size);
    item->key_size   = key_size;
    item->value_size = value_size;
    item->key_hash   = vfshash(key, key_size);
    item->ref        = 1;

    if (item->key == NULL || item->value == NULL) {
        goto cleanup;
    }

    memcpy(item->key, key, key_size);
    memcpy(item->value, value, value_size);

    hash_add_head(&hash_table_g, item->key_hash, &item->node);

    return 0;
cleanup:
    vfs_raise_err(3);
}

void *vfs_data_global_get(uint8_t *key, size_t key_size) {

    vfs_hash_t *tmp;
    int find_it = -1;

    hash_for_each_entry(tmp, &hash_table_g, _key_hash, node) {

        if (vfs_memcmp_n(tmp->key, tmp->key_size, key, key_size) == 0) {
            find_it = 1;
            break;
        }
    }

    if (find_it == -1) {
        return NULL;
    }

    if (tmp->ref == 0) {
        return NULL;
    }

    tmp->ref++;

    return tmp;
}

void vfs_data_global_get_done(uint8_t *key, size_t key_size) {

    vfs_hash_t *tmp;
    int find_it = -1;

    hash_for_each_entry(tmp, &hash_table_g, _key_hash, node) {

        if (vfs_memcmp_n(tmp->key, tmp->key_size, key, key_size) == 0) {
            find_it = 1;
            break;
        }
    }

    if (find_it == -1) {
        vfs_raise_err(4);
    });

    tmp->ref--;
    return tmp;
}

void *vfs_data_global_del_start(uint8_t *key, size_t key_size) {

    vfs_hash_t *tmp;
    int find_it = -1;

    hash_for_each_entry(tmp, &hash_table_g, _key_hash, node) {

        if (vfs_memcmp_n(tmp->key, tmp->key_size, key, key_size) == 0) {
            find_it = 1;
            break;
        }
    }

    if (find_it == -1 || tmp->ref != 1) {
        return NULL;
    }

    tmp->ref--;

    return tmp;
}

void vfs_data_global_del_rollback(uint8_t *key, size_t key_size) {

    vfs_hash_t *tmp;
    int find_it = -1;

    hash_for_each_entry(tmp, &hash_table_g, _key_hash, node) {

        if (vfs_memcmp_n(tmp->key, tmp->key_size, key, key_size) == 0) {
            find_it = 1;
            break;
        }
    }

    if (find_it == -1 || tmp->ref != 0) {
        vfs_raise_err(5);
    }

    tmp->ref++;

    return tmp;
}

void vfs_data_global_del_end(uint8_t *key, size_t key_size) {

    vfs_hash_t *tmp;
    int find_it = -1;

    hash_for_each_entry(tmp, &hash_table_g, _key_hash, node) {

        if (vfs_memcmp_n(tmp->key, tmp->key_size, key, key_size) == 0) {
            find_it = 1;
            break;
        }
    }

    if (find_it == -1) {
        vfs_raise_err(6);
    }

    hash_del_node(&hash_table_g, &tmp->node);

    vfs_free(tmp->key);
    vfs_free(tmp->value);
    vfs_free(tmp);
}

void *vfs_rwlock_init() {
    rwlock_t *lock = vfs_malloc(sizeof(rwlock_t));
    rwlock_init(lock, 1, "vfs");
    return lock;
}
void vfs_wlock_acquire(void *lock) {
    write_lock((rwlock_t *)lock);
}
void vfs_wlock_release(void *lock) {
    write_unlock((rwlock_t *)lock);
}
void vfs_rlock_acquire(void *lock) {
    read_lock((rwlock_t *)lock);
}
void vfs_rlock_release(void *lock) {
    read_unlock((rwlock_t *)lock);
}
void vfs_rwlock_deinit(void *lock) {
    vfs_free(lock);
}

void *vfs_get_process() {
    return &myproc()->task->vfs_process;
}
