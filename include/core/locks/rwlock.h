#ifndef __RWLOCK_H__
#define __RWLOCK_H__

#include "lib/fifo.h"
#include "core/locks/spinlock.h"

// 0: 读者优先  1：写者优先
#define RW_PRIOR (1 << 0)
typedef struct
{
    spinlock_t lock;
    int flags;
    int r_cnt;
    struct fifo readers_fifo;
    struct fifo writers_fifo;

    char name[16];  // for debug
}rwlock_t;
extern void rwlock_init(rwlock_t *rw, int prior, char *name);
extern void set_rw_prior(rwlock_t *rw, int prior);
extern void read_lock(rwlock_t *rw);
extern void read_unlock(rwlock_t *rw);
extern void write_lock(rwlock_t *rw);
extern void write_unlock(rwlock_t *rw);

#endif
