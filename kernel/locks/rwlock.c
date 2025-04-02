#include "core/proc.h"
#include "lib/fifo.h"
#include "lib/spinlock.h"
// 0: 读者优先  1：写者优先
#define RW_PRROR (1 << 0)
// 1. 正在写
#define WRITING (1 << 1)
struct rwlock
{
    spinlock_t lock;
    int flags;
    int r_cnt;
    struct fifo readers_fifo;
    struct fifo writers_fifo;

    char name[16];  // for debug
};

static void __sleep_fifo(struct thread_info *t, struct fifo *fifo)
{
    assert(list_empty(&t->sched), "__sleep_fifo list_empty(&t->sched)\n");
    t->state = SLEEPING;
    fifo_push(fifo, &t->sched);
}

static void __wakeup_fifo(struct fifo *fifo)
{
    if (fifo_empty(fifo))
        return;
    struct list_head *waiter = fifo_pop(fifo);
    if (waiter != NULL) {
        struct thread_info *thread = list_entry(waiter, struct thread_info, sched);
        wakeup_process(thread);
    }
    else {
        panic("__wakeup_fifo: queue may have a race condition. "
              "all size: %d, but queue-len: %d\n",
              fifo_size(fifo),
              list_len(&fifo->f_list));
    }
}

// 这四个必须在持有 rw->lock 情况下调用
static inline void wakeup_reader(struct rwlock *rw)
{
    __wakeup_fifo(&rw->readers_fifo);
}

static inline void wakeup_writer(struct rwlock *rw)
{
    // 需要设置标志，防止换醒后，释放锁后，其他读线程争夺
    SET(WRITING);
    __wakeup_fifo(&rw->writers_fifo);
}

static void sleep_reader(struct thread_info *t, struct rwlock *rw)
{
    spin_lock(&t->lock);
    __sleep_fifo(t, &rw->readers_fifo);
    spin_unlock(&rw->lock);
    sched();

    spin_lock(&rw->lock);
}

static void sleep_writer(struct thread_info *t, struct rwlock *rw)
{
    spin_lock(&t->lock);
    __sleep_fifo(t, &rw->writers_fifo);
    spin_unlock(&rw->lock);
    sched();

    spin_lock(&rw->lock);
}

void read_lock(struct rwlock *rw)
{
    struct thread_info *t = myproc();
    spin_lock(&rw->lock);
    if (TEST(WRITING)) {      // 有写者
        sleep_reader(t, rw);  // 在锁上睡眠
        wakeup_reader(t);     // 醒来后级联唤醒
    }
    // 读者优先的话，直接操作运行就好

    // 写者优先，如果有等待写者，需要堵塞读者
    if (RW_PRROR == 1 && !fifo_empty(&rw->writers_fifo)) {
        sleep_reader(t, rw);  // 在锁上睡眠
    }
    // 理论上不会存在写者优先，没有写者在写，但是写者队列不为空的情况
    // 因为我们在 wakeup_writer 中是先标记有写者，把其他读线程挡住，再把写者唤醒
    rw->r_cnt++;
    spin_unlock(&rw->lock);
}

void read_unlock(struct rwlock *rw)
{
    spin_lock(&rw->lock);
    rw->r_cnt--;
    if (rw->r_cnt == 0 && !fifo_empty(&rw->writers_fifo))  // 没有读者，且有写者在等待，则唤醒写者
        wakeup_writer(rw);
    spin_unlock(&rw->lock);
}

void write_lock(struct rwlock *rw)
{
    struct thread_info *t = myproc();
    spin_lock(&rw->lock);
    if (rw->r_cnt != 0 || TEST(WRITING)) {  // 当前有人再用
        sleep_writer(t, rw);
    }
    SET(WRITING);
    spin_unlock(&rw->lock);
}

void write_unlock(struct rwlock *rw)
{
    spin_lock(&rw->lock);
    CLEAR(WRITING);
    if (RW_PRROR == 0 || fifo_empty(&rw->writers_fifo))  // 读者优先,或者没有写者
        wakeup_reader(rw);
    else  // 写者优先，唤醒一个写者
        wakeup_writer(rw);
    spin_unlock(&rw->lock);
}
