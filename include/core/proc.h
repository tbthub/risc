#ifndef __PROC_H__
#define __PROC_H__

#include "core/sched.h"
#include "core/signal.h"
#include "core/locks/semaphore.h"
#include "core/locks/spinlock.h"
#include "lib/list.h"
#include "lib/hash.h"
#include "param.h"
#include "riscv.h"
#include "std/stddef.h"
#include "vm.h"
#include "vfs/vfs_process.h"

// Saved registers for kernel context switches.
struct context {
    uint64_t ra;
    uint64_t sp;

    // callee-saved
    uint64_t s0;
    uint64_t s1;
    uint64_t s2;
    uint64_t s3;
    uint64_t s4;
    uint64_t s5;
    uint64_t s6;
    uint64_t s7;
    uint64_t s8;
    uint64_t s9;
    uint64_t s10;
    uint64_t s11;

};

// Per-CPU state.
struct cpu {
    struct context context; // swtch() here to enter scheduler().
    int noff;               // Depth of push_off() nesting.
    int intena;             // Were interrupts enabled before push_off()?

    // xv6中使用thread是为了方便找到当前进程。但是我们采取内核栈与PCB共占一个页面，也就是sp指针来确定即可
    // 暂时先不管
    struct thread_info *thread;
    struct thread_info *idle;
    struct sched_struct sched_list; // 每个CPU的进程调用链
};
extern struct cpu cpus[NCPU];

// 线程状态
enum task_state {
    UNUSED,
    USED,
    SLEEPING,
    RUNNABLE,
    RUNNING,
    ZOMBIE,
    IDLE
};

struct thread_info;

// struct th_table_struct
// {
//     // 位为 0 即为主线程
//     struct thread_info **array;
//     spinlock_t lock;
//     semaphore_t main_thread_wait;
//     int num;
//     int alloc;
// };

// 主要管理资源，文件等。对个thread_info对应一个task_struct。操作task_struct时候需要加锁
struct task_struct {
    spinlock_t lock;
    struct mm_struct mm;
    struct signal sigs;

    vfs_process_t vfs_proc;

    // struct th_table_struct th_table;  // TODO fork copy
};

struct trapframe {
    /*   0 */ uint64_t kernel_sp;
    /*   8 */ uint64_t epc;

    /*  16 */ uint64_t ra;
    /*  24 */ uint64_t sp;
    /*  32 */ uint64_t gp;
    /*  40 */ uint64_t tp;
    /*  48 */ uint64_t t0;
    /*  56 */ uint64_t t1;
    /*  64 */ uint64_t t2;
    /*  72 */ uint64_t s0;
    /*  80 */ uint64_t s1;
    /*  88 */ uint64_t a0;
    /*  96 */ uint64_t a1;
    /* 104 */ uint64_t a2;
    /* 112 */ uint64_t a3;
    /* 120 */ uint64_t a4;
    /* 128 */ uint64_t a5;
    /* 136 */ uint64_t a6;
    /* 144 */ uint64_t a7;
    /* 152 */ uint64_t s2;
    /* 160 */ uint64_t s3;
    /* 168 */ uint64_t s4;
    /* 176 */ uint64_t s5;
    /* 184 */ uint64_t s6;
    /* 192 */ uint64_t s7;
    /* 200 */ uint64_t s8;
    /* 208 */ uint64_t s9;
    /* 216 */ uint64_t s10;
    /* 224 */ uint64_t s11;
    /* 232 */ uint64_t t3;
    /* 240 */ uint64_t t4;
    /* 248 */ uint64_t t5;
    /* 256 */ uint64_t t6;

};

struct thread_info {
    tid_t tid;
    struct task_struct *task;

    spinlock_t lock;

    // p->lock must be held when using these:
    uint32_t flags;
    enum task_state state;
    pid_t pid;
    uint64_t ticks;

    // wait_lock must be held when using this:
    struct thread_info *parent; // 父进程指针
    struct list_head child;
    struct list_head sibling;
    struct list_head sched; // 全局任务链表, 同时也是信号量等待队列

    semaphore_t child_exit_sem;

    hash_node_t global;

    struct trapframe *tf;

    void (*func)(void *);
    void *args;
    int cpu_affinity;

    uint16_t cpu_id; // 用于记录当前线程在哪个核心上运行，这个暂时没想到干啥，先用着

    int exit_code;

    // these are private to the process, so p->lock need not be held.
    struct context context;
    char name[16];
};

extern void proc_init();
extern int cpuid();
extern struct cpu *mycpu();
extern struct thread_info *myproc(void);
extern struct thread_info *find_proc(int _pid);
extern struct thread_info *kthread_struct_init();
extern struct thread_info *uthread_struct_init();
extern struct thread_info *get_init();

#define KERNEL_STACK_TOP(t) ((uint64_t)t + 2 * PGSIZE - 16)

#endif
