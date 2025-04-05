#include "lib/hash.h"
#include "lib/string.h"
#include "vfs_interface.h"

typedef struct vfs_hash_t {
    uint8_t *key;
    size_t key_size;
    uint8_t *value;
    size_t value_size;
    hash_node_t node;
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

int8_t vfs_data_global_put(uint8_t *key, size_t key_size, void *value, size_t value_size) {

    vfs_hash_t *tmp;
    hash_find(tmp, &hash_table_g, value, vfshash(key, key_size), node);
    
    if (tmp != NULL){
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
    item->ref        = 1;

    if (item->key == NULL || item->value == NULL){
        goto cleanup;
    }

    memcpy(item->key, key, key_size);
    memcpy(item->value, value, value_size);

    hash_add_head(&hash_table_g, vfshash(key, key_size), &item->node);

    return 0;
cleanup:
    vfs_raise_err(3);
}
