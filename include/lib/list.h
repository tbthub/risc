#ifndef __LIST_H__
#define __LIST_H__
#include "std/stddef.h"
#include "std/stdio.h"

// 循环双链表
struct list_head
{
    struct list_head *prev;
    struct list_head *next;
};

#define INIT_LIST_HEAD(ptr)  \
    do                       \
    {                        \
        (ptr)->next = (ptr); \
        (ptr)->prev = (ptr); \
    } while (0)

#define LIST_HEAD_INIT(name) {&(name), &(name)}

#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_reverse(pos, head) \
    for (pos = (head)->prev; pos != (head); pos = pos->prev)

#define list_for_each_entry(pos, head, member)                 \
    for (pos = list_entry((head)->next, typeof(*pos), member); \
         &pos->member != (head);                               \
         pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_reverse(pos, head, member)         \
    for (pos = list_entry((head)->prev, typeof(*pos), member); \
         &pos->member != (head);                               \
         pos = list_entry(pos->member.prev, typeof(*pos), member))

#define list_for_each_safe(pos, tmp, head) \
    for (pos = (head)->next, tmp = pos->next; pos != (head); pos = tmp, tmp = pos->next)

#define list_for_each_entry_safe(pos, tmp, head, member)          \
    for (pos = list_entry((head)->next, typeof(*pos), member),    \
        tmp = list_entry(pos->member.next, typeof(*pos), member); \
         &pos->member != (head);                                  \
         pos = tmp, tmp = list_entry(pos->member.next, typeof(*pos), member))

static inline void __list_add(struct list_head *_new, struct list_head *prev, struct list_head *next)
{
    prev->next = next->prev = _new;
    _new->next = next;
    _new->prev = prev;
}

static inline void __list_del(struct list_head *prev, struct list_head *next)
{
    prev->next = next;
    next->prev = prev;
}

static inline void list_del(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
}

static inline void list_del_init(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
    INIT_LIST_HEAD(entry);
}

static inline void list_add_tail(struct list_head *_new, struct list_head *list)
{
    __list_add(_new, list->prev, list);
}

static inline void list_add_head(struct list_head *_new, struct list_head *list)
{
    __list_add(_new, list, list->next);
}

static inline int list_is_first(const struct list_head *list, const struct list_head *head)
{
    return list->prev == head;
}

static inline int list_is_last(const struct list_head *list, const struct list_head *head)
{
    return list->next == head;
}

static inline int list_empty(const struct list_head *list)
{
    return list->next == list;
}

static inline struct list_head *list_first(const struct list_head *list)
{
    return list->next;
}

static inline void list_splice_head(struct list_head *old_list,struct list_head *new_list)
{
    old_list->prev->next = new_list;
    old_list->next->prev = new_list->next;
    new_list->next->prev = old_list->next;
    new_list->next = old_list->prev;
    old_list->next = old_list->prev = NULL;
}



static inline struct list_head *list_pop(struct list_head *list)
{
    if (list_empty(list))
        return NULL;

    struct list_head *tmp = list->next;
    list_del_init(list->next);
    return tmp;
}

static inline uint32_t list_len(struct list_head *list)
{
    uint32_t len = 0;
    struct list_head *pos;
    list_for_each(pos, list)
    {
        len++;
        if (len > 1000000)
        {
            panic("\n-------maybe len max-------\n");
        }
    }
    return len;
}

#endif
