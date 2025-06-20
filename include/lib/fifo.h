#ifndef __FIFI_H__
#define __FIFI_H__
#include "lib/list.h"
#include "std/stddef.h"
#include "lib/list.h"
#include "std/stdio.h"

struct fifo
{
    int f_size;
    struct list_head f_list;
};

static inline void fifo_init(struct fifo *fifo)
{
    INIT_LIST_HEAD(&fifo->f_list);
    fifo->f_size = 0;
}

static inline void fifo_push(struct fifo *fifo, struct list_head *entry)
{
    assert(fifo != NULL ,"fifo\n");
    assert(entry != NULL ,"entry\n");
    list_add_tail(entry, &fifo->f_list);
    fifo->f_size++;
}

static inline int fifo_empty(const struct fifo *fifo)
{
    return fifo->f_size == 0;
}

static inline struct list_head *fifo_pop(struct fifo *fifo)
{
    if (list_empty(&fifo->f_list))
        return NULL;
    fifo->f_size--;
    struct list_head * list = list_pop(&fifo->f_list);
    return list;
}

static inline int fifo_size(const struct fifo *fifo)
{
    return fifo->f_size;
}

#endif
