#include "core/locks/spinlock.h"

#include "core/proc.h"
#include "defs.h"
#include "lib/string.h"
#include "riscv.h"
#include "std/stddef.h"

void spin_init(spinlock_t *lock, const char *name)
{
    lock->lock = SPIN_UNLOCKED;
    // strncpy(lock->name, name, 12);
    strdup(lock->name, name);
    lock->cpuid = -1;
}

// push_off/pop_off are like intr_off()/intr_on() except that they are matched:
// it takes two pop_off()s to undo two push_off()s.  Also, if interrupts
// are initially off, then push_off, pop_off leaves them off.
// 关中断、增加当前 CPU 的中断层数
void push_off()
{
    int old = intr_get();

    intr_off();
    if (mycpu()->noff == 0)
        mycpu()->intena = old;
    mycpu()->noff += 1;
}

// 开中断、减少当前 CPU 的中断层数
void pop_off()
{
    uint64 sp1 = r_sp();
    // 如果中断已经打开
    if (intr_get()) {
        panic("spinlock pop_off: intr on! hart: %d\n", cpuid());
    }
    struct cpu *c = mycpu();
    // 理论上不会发生这个情况
    if (c->noff < 1)
        panic("spinlock pop_off: cpu->noff less than 1! hart: %d\n", cpuid());

    c->noff -= 1;
    if (c->noff == 0 && c->intena) {
        intr_on();
    }

    uint64 sp2 = r_sp();
    assert(sp1 == sp2, "sp\n");
}

// 需要在进制中断的情况下调用（cpu_id）
int holding(spinlock_t *lock)
{
    return lock->lock == SPIN_LOCKED && lock->cpuid == cpuid();
}

void spin_lock(spinlock_t *lock)
{
    push_off();
    if (holding(lock))
        panic("spinlock: already held when trying to lock\n - name: %s, cpu: %d, thread name: %s\n", lock->name, cpuid(), myproc()->name);

    // 原子交换
    // On RISC-V, sync_lock_test_and_set turns into an atomic swap:
    //   a5 = 1 (SPIN_LOCK_LOCKED)
    //   s1 = locked
    //   amoswap.w.aq a5, a5, (s1)
    while (__sync_lock_test_and_set(&lock->lock, SPIN_LOCKED) != 0)
    ;
    lock->cpuid = cpuid();
    // Tell the C compiler and the processor to not move loads or stores
    // past this point, to ensure that the critical section's memory
    // references happen strictly after the lock is acquired.
    // On RISC-V, this emits a fence instruction.
    __sync_synchronize();
}

void spin_unlock(spinlock_t *lock)
{
    if (!holding(lock))
        panic("spinlock: not held when trying to unlock, name: %s, cpu: %d\n", lock->name, cpuid());

    // Tell the C compiler and the CPU to not move loads or stores
    // past this point, to ensure that all the stores in the critical
    // section are visible to other CPUs before the lock is released,
    // and that loads in the critical section occur strictly before
    // the lock is released.
    // On RISC-V, this emits a fence instruction.
    __sync_synchronize();

    // Release the lock, equivalent to lk->locked = 0.
    // This code doesn't use a C assignment, since the C standard
    // implies that an assignment might be implemented with
    // multiple store instructions.
    // On RISC-V, sync_lock_release turns into an atomic swap:
    //   s1 = lock
    //   amoswap.w zero, zero, (lock)
    lock->cpuid = -1;
    __sync_lock_release(&lock->lock);
    pop_off();
}
