#include "lib/hash.h"
#include "std/string.h"
#include "mm/kmalloc.h"
#include "mm/mm.h"
#include "core/locks/rwlock.h"
#include "core/proc.h"
#include "vfs/vfs_interface.h"
#include "vfs/vfs_module.h"
#include "std/string.h"
#include "lib/math.h"
#include "std/stdio.h"

typedef struct vfs_hash_t {
    uint8_t *key;
    size_t key_size;
    uint8_t *value;
    size_t value_size;
    hash_node_t node;
    int key_hash;
    int ref;
} vfs_hash_t;

typedef struct vfs_alloc_t {
    uint32_t malloc_size;
    uint32_t order;
} vfs_alloc_t;

static struct hash_table hash_table_g;

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

    size = size + sizeof(vfs_alloc_t);

    assert(size <= 1024 * 32, "vfs_malloc too big! %d\n", size);
    uint8_t *ptr = NULL;
    size_t order = 0;
    if (size >= 8192) {
        size_t page_size = (size + 4095) / 4096;
        order            = math_log(page_size, 2);
        ptr              = (uint8_t *)__alloc_pages(0, order);
    }

    else {
        ptr = (uint8_t *)kmalloc((int)size, 0);
    }

    assert(ptr != NULL, "vfs_malloc NULL!\n");

    vfs_alloc_t *ptr_head = (vfs_alloc_t *)ptr;
    ptr_head->malloc_size = (uint32_t)size;
    ptr_head->order       = (uint32_t)order;

    return ptr + sizeof(vfs_alloc_t);
}

void vfs_free(void *ptr) {
    uint8_t *data         = (uint8_t *)ptr;
    vfs_alloc_t *ptr_head = (vfs_alloc_t *)(data - sizeof(vfs_alloc_t));

    if (ptr_head->malloc_size >= 8192) {
        __free_pages((void *)ptr_head, ptr_head->order);
    }

    else {
        kfree((void *)ptr_head);
    }
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
    return -1;
}

void *vfs_data_global_get(uint8_t *key, size_t key_size) {

    vfs_hash_t *tmp;
    int find_it = -1;

    hash_for_each_entry(tmp, &hash_table_g, vfshash(key, key_size), node) {

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

    return tmp->value;
}

void vfs_data_global_get_done(uint8_t *key, size_t key_size) {

    vfs_hash_t *tmp;
    int find_it = -1;

    hash_for_each_entry(tmp, &hash_table_g, vfshash(key, key_size), node) {

        if (vfs_memcmp_n(tmp->key, tmp->key_size, key, key_size) == 0) {
            find_it = 1;
            break;
        }
    }

    if (find_it == -1) {
        vfs_raise_err(4);
    };

    tmp->ref--;
}

void *vfs_data_global_del_start(uint8_t *key, size_t key_size) {

    vfs_hash_t *tmp;
    int find_it = -1;

    hash_for_each_entry(tmp, &hash_table_g, vfshash(key, key_size), node) {

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

    hash_for_each_entry(tmp, &hash_table_g, vfshash(key, key_size), node) {

        if (vfs_memcmp_n(tmp->key, tmp->key_size, key, key_size) == 0) {
            find_it = 1;
            break;
        }
    }

    if (find_it == -1 || tmp->ref != 0) {
        vfs_raise_err(5);
    }

    tmp->ref++;
}

void vfs_data_global_del_end(uint8_t *key, size_t key_size) {

    vfs_hash_t *tmp;
    int find_it = -1;

    hash_for_each_entry(tmp, &hash_table_g, vfshash(key, key_size), node) {

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
    return &myproc()->task->vfs_proc;
}

// 根文件设备，需要初始化设备后才能

void vfs_init() {
    hash_init(&hash_table_g, 128, "vfs_tag");

    // extern const uint8_t init_code;
    // extern const size_t init_code_size;

    // assert(vfs_mkfs_add_file(&simfs_table, "/init.elf", &init_code, init_code_size) >= 0, "vfs_mkfs_add_file failed!");
}
