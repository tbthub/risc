#include "core/locks/rwlock.h"

#include "core/proc.h"
#include "std/string.h"

// 1. 正在写
#define WRITING (1 << 1)

static void __sleep_fifo(struct thread_info *t, struct fifo *fifo) {
    assert(list_empty(&t->sched), "__sleep_fifo list_empty(&t->sched)\n");
    t->state = SLEEPING;
    fifo_push(fifo, &t->sched);
}

static void __wakeup_fifo(struct fifo *fifo) {
    struct list_head *waiter = fifo_pop(fifo);
    if (waiter != NULL) {
        struct thread_info *thread = list_entry(waiter, struct thread_info, sched);
        wakeup_process(thread);
    } else {
        panic("__wakeup_fifo: queue may have a race condition. "
              "all size: %d, but queue-len: %d\n",
              fifo_size(fifo),
              list_len(&fifo->f_list));
    }
}

/*
 * 获取当前的优先级方式
 * 0: 读者优先
 * != 0：写者优先
 */
static int get_rw_prior(rwlock_t *rw) {
    return rw->flags & RW_PRIOR;
}

void set_rw_prior(rwlock_t *rw, int prior) {
    spin_lock(&rw->lock);
    SET_FLAG(&rw->flags, prior);
    spin_unlock(&rw->lock);
}

// 这四个必须在持有 rw->lock 情况下调用
static inline void wakeup_reader(rwlock_t *rw) {
    if (fifo_empty(&rw->readers_fifo))
        return;
    __wakeup_fifo(&rw->readers_fifo);
}

static inline void wakeup_writer(rwlock_t *rw) {
    if (fifo_empty(&rw->writers_fifo))
        return;
    // 需要设置标志，防止换醒后，释放锁后，其他读线程争夺
    SET_FLAG(&rw->flags, WRITING);
    __wakeup_fifo(&rw->writers_fifo);
}

static void sleep_reader(struct thread_info *t, rwlock_t *rw) {
    spin_lock(&t->lock);
    __sleep_fifo(t, &rw->readers_fifo);
    spin_unlock(&rw->lock);
    sched();

    spin_lock(&rw->lock);
}

static void sleep_writer(struct thread_info *t, rwlock_t *rw) {
    spin_lock(&t->lock);
    __sleep_fifo(t, &rw->writers_fifo);
    spin_unlock(&rw->lock);
    sched();

    spin_lock(&rw->lock);
}

void read_lock(rwlock_t *rw) {
    struct thread_info *t = myproc();
    spin_lock(&rw->lock);

    // 需要重新检查，sleep_reader虽然会重新获取自旋锁，但是如果在
    // 调度回来，获取自旋锁之间，有新的写者获取到自旋锁的情况
    // 因此这里需要重新转圈
retry:
    if (TEST_FLAG(&rw->flags, WRITING) == WRITING) { // 有写者
        sleep_reader(t, rw);                         // 在锁上睡眠
        goto retry;
    }
    // 读者优先的话，直接操作运行就好
    // 写者优先，如果有等待写者，需要堵塞读者
    if (get_rw_prior(rw) != 0 && !fifo_empty(&rw->writers_fifo)) {
        printk("read_lock write prior, reader %d sleep\n", myproc()->pid);
        sleep_reader(t, rw); // 在锁上睡眠
        goto retry;
    }

    wakeup_reader(rw); // 醒来后级联唤醒（队列为空也没事）

    // 理论上不会存在写者优先，没有写者在写，但是写者队列不为空的情况
    // 因为我们在 wakeup_writer 中是先标记有写者，把其他读线程挡住，再把写者唤醒
    rw->r_cnt++;
    spin_unlock(&rw->lock);
}

void read_unlock(rwlock_t *rw) {
    spin_lock(&rw->lock);
    rw->r_cnt--;
    if (rw->r_cnt == 0 && !fifo_empty(&rw->writers_fifo)) // 没有读者，且有写者在等待，则唤醒写者
        wakeup_writer(rw);
    spin_unlock(&rw->lock);
}

void write_lock(rwlock_t *rw) {
    struct thread_info *t = myproc();
    spin_lock(&rw->lock);

    if (rw->r_cnt != 0 || TEST_FLAG(&rw->flags, WRITING) == WRITING) { // 当前有人再用
        sleep_writer(t, rw);
    }
    SET_FLAG(&rw->flags, WRITING);
    spin_unlock(&rw->lock);
}

void write_unlock(rwlock_t *rw) {
    spin_lock(&rw->lock);
    CLEAR_FLAG(&rw->flags, WRITING);
    // 读者优先，则优先唤醒读者；没有读者，唤醒写者
    // 写者优先，则优先唤醒写者；没有写者，唤醒读者
    int res = (get_rw_prior(rw) == 0 && !fifo_empty(&rw->readers_fifo)) || // 读者优先且有读者等待 ->唤醒读者
              (get_rw_prior(rw) != 0 && fifo_empty(&rw->writers_fifo));    // 写者优先但无写者等待 ->唤醒读者（可能，但是队列为空没问题）
    printk("write_unlock rw: %d, r_len: %d, w_len: %d\n",
           get_rw_prior(rw),
           fifo_size(&rw->readers_fifo),
           fifo_size(&rw->writers_fifo));

    if (res) {
        // printk("write_unlock wakeup_reader\n");
        wakeup_reader(rw);
    } else {
        // printk("write_unlock wakeup_writer\n");
        wakeup_writer(rw);
    }

    spin_unlock(&rw->lock);
}

void rwlock_init(rwlock_t *rw, int prior, char *name) {
    spin_init(&rw->lock, "rwlock");
    set_rw_prior(rw, prior);
    rw->r_cnt = 0;
    fifo_init(&rw->readers_fifo);
    fifo_init(&rw->writers_fifo);
    strncpy(rw->name, name, 16);
}
