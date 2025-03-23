#include "lib/hash.h"
#include "mm/kmalloc.h"
#include "lib/string.h"



inline int hash_init(struct hash_table *htable, int size, const char *name)
{
    strdup(htable->name, name);
    htable->size = size;
    htable->count = 0;
    htable->heads = (struct list_head *)kmalloc(sizeof(struct list_head) * size, 0);
    if (!htable->heads)
    {
        printk("hlist_init heads kmalloc error\n");
        return 0;
    }
    for (int i = 0; i < size; i++)
        INIT_LIST_HEAD(&htable->heads[i]);
    return 1;
}

inline void hash_free(struct hash_table *htable)
{
    if (!htable)
        return;
    kfree(htable->heads);
}
