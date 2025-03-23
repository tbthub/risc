#ifndef __htable_H__
#define __htable_H__
#include "std/stddef.h"
#include "mm/kmalloc.h"
#include "lib/list.h"

typedef struct list_head hash_node_t;

// 哈希表
// 对其操作必须互斥
struct hash_table
{
    int size; // 哈希表大小
    int count;
    char name[12];
    hash_node_t *heads;
};

extern int hash_init(struct hash_table *htable, int size, const char *name);
extern void hash_free(struct hash_table *htable);

#define INIT_HASH_NODE(entry) \
    INIT_LIST_HEAD(entry)

#define _hash(htable, key) key % (htable)->size

#define hash_empty(htable, key) \
    list_empty(&((htable)->heads[_hash(htable, key)]))

#define hash_add_tail(htable, key, list)                             \
    do                                                               \
    {                                                                \
        list_add_tail(list, &((htable)->heads[_hash(htable, key)])); \
        (htable)->count++;                                           \
    } while (0)

#define hash_add_head(htable, key, list)                             \
    do                                                               \
    {                                                                \
        list_add_head(list, &((htable)->heads[_hash(htable, key)])); \
        (htable)->count++;                                           \
    } while (0)

#define hash_for_each(pos, htable, key)                               \
    struct list_head *__hhead = &((htable)->heads[_hash(htable, key)]); \
    list_for_each(pos, __hhead)

#define hash_for_each_entry(pos, htable, key, member)                 \
    struct list_head *__hhead = &((htable)->heads[_hash(htable, key)]); \
    list_for_each_entry(pos, __hhead, member)

#define hash_find(pos, htable, key_member, key, hnode_member)                             \
    do                                                                                    \
    {                                                                                     \
        int __hash_found = 0;                                                             \
        hash_for_each_entry(pos, htable, key, hnode_member) if (pos->key_member == key) \
        {                                                                                 \
            __hash_found = 1;                                                             \
            break;                                                                        \
        }                                                                                 \
        if (!__hash_found)                                                                \
            pos = NULL;                                                                   \
    } while (0)

#define hash_del_node(htable, entry) \
    do                               \
    {                                \
        list_del(entry);             \
        (htable)->count--;           \
    } while (0)

#define hash_del(pos, htable, key_member, key, hnode_member)   \
    do                                                         \
    {                                                          \
        hash_find(pos, htable, key_member, key, hnode_member); \
        if (pos)                                               \
        {                                                      \
            list_del(&pos->hnode_member);                      \
            (htable)->count--;                                 \
        }                                                      \
    } while (0)

#define hash_debug(htable)                                                        \
    do                                                                            \
    {                                                                             \
        printk("\n------ hash debug start ------\n");                             \
        for (int _i = 0; _i < (htable)->size; _i++)                               \
            printk("hash[%d] count: %d\n", _i, list_len(&((htable)->heads[_i]))); \
        printk("------ hash debug end ------\n");                                 \
    } while (0)

#endif
