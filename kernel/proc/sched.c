#include "core/sched.h"

#include "core/proc.h"
#include "core/vm.h"
#include "lib/atomic.h"
#include "lib/spinlock.h"
#include "lib/string.h"
#include "riscv.h"
#include "std/stddef.h"

// 0号进程也就是第一个内核线程，负责初始化部分内容后作为调度器而存在

// 为每一个cpu分配一个调度队列
// 在顶层以后可以加一个负载均衡的分配器
// 负责分发不同的任务，现在为了简单就直接轮转加入

// 负载均衡器，这里暂时轮转分配(有点性能问题)
spinlock_t load_lock;
int load_cpu_id = -1;
extern struct thread_info *init_t;

// switch.S
extern void swtch(struct context *, struct context *);

// 从调度队列中选择下一个要执行的线程
// 如果有其他进程，则选择其他的
static struct thread_info *pick_next_task(struct list_head *run_list)
{
    struct thread_info *thread;
    if (list_empty(run_list))
        return NULL;
    // if (cpuid() == 0)
    // {
    //     printk("cpu0: sched len:%d\n", list_len(run_list));
    // }
    list_for_each_entry_reverse(thread, run_list, sched)
    {
        // 找到一个可以运行的线程
        if (thread->state == RUNNABLE)
            return thread;
    }
    return NULL;
}

static void add_runnable_task(struct thread_info *thread)
{
    int cpuid;
    int cpu_affinity = thread->cpu_affinity;

    spin_lock(&load_lock);
    if (cpu_affinity == NO_CPU_AFF)
        cpuid = load_cpu_id = (load_cpu_id + 1) % NCPU;
    else
        cpuid = cpu_affinity;
#ifdef DEBUG_TASK_ADD_CPU
    printk("add thread %s to cpu %d\n", thread->name, cpuid);
#endif
    spin_unlock(&load_lock);

    spin_lock(&cpus[cpuid].sched_list.lock);
    list_add_head(&thread->sched, &(cpus[cpuid].sched_list.run));
    spin_unlock(&cpus[cpuid].sched_list.lock);
}

void sched(int old_intr)
{
    int intena;
    struct thread_info *thread = myproc();

    assert(holding(&thread->lock) != 0, "sched: not held p->lock");                                      // 确保持有锁
    assert(thread->state != RUNNING, "sched: '%s' is running, state: %d", thread->name, thread->state);  // 确保进程不在运行态
    assert(intr_get() == 0, "sched interruptible");                                                      // 确保关中断

    intena = mycpu()->intena;
    // printk("Thread %s switch to scheduler in sched\n", thread->name);
    swtch(&thread->context, &mycpu()->context);
    printk("sched2-%d,hart: %d\n", thread->pid,thread->cpu_id);

    assert(intr_get() == 0, "sched intr_get\n");
    // printk("sched3-%d\n", thread->pid);

    // printk("Thread %s releasing lock on cpu %d ret sched\n", thread->name, cpuid());

    if (thread->tf) {
        spin_lock(&thread->task->mm.lock);
        // 如果发生了进程切换
        if (r_satp() != MAKE_SATP(thread->task->mm.pgd)) {
            sfence_vma();
            w_satp(MAKE_SATP(thread->task->mm.pgd));
            sfence_vma();
        }
        spin_unlock(&thread->task->mm.lock);
    }

    mycpu()->intena = intena;
    printk("sched4-%d hart: %d\n\n", thread->pid,cpuid());
    spin_unlock(&thread->lock);
    // 回来默认是关中断的
    if(old_intr == 1){
        intr_on();
    }
}

// 从进程上下文切换到调度线程（主线程）, 进程交换上下文后中断返回
void yield()
{
    // printk("intr status: %d\n",intr_get());
    // printk("-----------timer interrupt yield!!!--------------\n");
    int intr = intr_get();
    struct thread_info *thread = myproc();
    // printk("Thread %s acquiring lock on cpu %d in yield\n", thread->name, cpuid());
    spin_lock(&thread->lock);
    thread->ticks = 10;
    thread->state = RUNNABLE;
    add_runnable_task(thread);
    // list_add_tail(&thread->sched, &mycpu()->sched_list.run);
    sched(intr);
    // printk("sched5-%d (yield)\n", thread->pid);
}

void scheduler()
{
    struct cpu *cpu = mycpu();
    intr_on();
    while (1) {
        intr_off();
        spin_lock(&cpu->sched_list.lock);
        struct thread_info *next = pick_next_task(&cpu->sched_list.run);
        if (next) {
            // 找到下一个线程，从就绪列表摘下
            // 原则上每个CPU有自己的调度队列，不会出现多个CPU同时调度一个线程的情况
            // 但是我们走 sleep 时候要主动让出CPU，在信号量中有对进程加锁，因此这里交换回来要解锁
            // 同理,我们在交换出去的时候加锁
            printk("Thread %s acquiring lock on cpu %d in scheduler\n", next->name, cpuid());
            spin_lock(&next->lock);
            printk("pick thread name: %s\n", next->name);
            list_del_init(&next->sched);
            spin_unlock(&cpu->sched_list.lock);

            next->state = RUNNING;
            next->cpu_id = cpuid();
            cpu->thread = next;
#ifdef DEBUG_TASK_ON_CPU
            printk("pid: %d, \tname: %s, in running on hart %d\n", next->pid, next->name, cpuid());
#endif
            // printk("sched1-%d\n", next->pid);
            swtch(&cpu->context, &next->context);

            // 线程已经在运行了
            // printk("thread: %s return..scheduler, intr: %d\n",next->name,intr_get());
            // 下面是被调度返回了调度器线程
            spin_unlock(&next->lock);
            // printk("Thread %s releasing lock on cpu %d in scheduler,intr: %d\n", next->name, cpuid(),intr_get());
            cpu->thread = NULL;
        }
        // 如果没有一个可以运行的进程，则运行idle
        else {
            spin_unlock(&cpu->sched_list.lock);
            intr_on();
            asm volatile("wfi");
        }
    }
}

void sched_init()
{
    spin_init(&load_lock, "load_lock");
    // 每个CPU都有一个指针指向这个idle
    for (int i = 0; i < NCPU; i++) {
        spin_init(&cpus[i].sched_list.lock, "sched_list");
        INIT_LIST_HEAD(&cpus[i].sched_list.run);
        INIT_LIST_HEAD(&cpus[i].sched_list.out);
    }
}

void wakeup_process(struct thread_info *thread)
{
    // printk("Thread %s acquiring lock on cpu %d in wakeup\n", thread->name, cpuid());
    spin_lock(&thread->lock);

    // 确保不在任何队列中，我们要将其加入到就绪队列
    assert(list_len(&thread->sched) == 0, "wakeup_process\n");

    thread->state = RUNNABLE;
    add_runnable_task(thread);
    spin_unlock(&thread->lock);
    // printk("Thread %s releasing lock on cpu %d in wakeup\n", thread->name, cpuid());
}

struct thread_info *kthread_create(void (*func)(void *), void *args, const char *name, int cpu_affinity)
{
    if (cpu_affinity < NO_CPU_AFF || cpu_affinity >= NCPU) {
        printk("Please pass in a suitable cpu_affinity value, "
               "the default value is 'NO_CPU_AFF'.\n"
               "The value you pass in is %d\n",
               cpu_affinity);
        return NULL;
    }
    struct thread_info *t = kthread_struct_init();
    if (!t) {
        printk("kthread_create\n");
        return NULL;
    }
    // init 线程的生命周期最长，且是第一个线程,即其 parent = NULL
    // 其他所有内核线程的 parent 能确保不为空
    t->parent = myproc();
    if (t->parent) {
        spin_lock(&t->parent->lock);
        list_add_head(&t->sibling, &t->parent->child);
        spin_unlock(&t->parent->lock);
    }

    t->func = func;
    t->args = args;
    strncpy(t->name, name, 16);
    t->cpu_affinity = cpu_affinity;

    wakeup_process(t);
    return t;
}

int64 do_pause()
{
    int intr = intr_get();
    struct thread_info *thread = myproc();
    spin_lock(&thread->lock);
    thread->state = RUNNABLE;
    add_runnable_task(thread);
    sched(intr);
    return 0;
}

__attribute__((unused)) void debug_cpu_shed_list()
{
    printk("\nCPU shed list:\n");
    for (int i = 0; i < NCPU; i++) {
        printk("cpu %d len %d\n", i, list_len(&cpus[i].sched_list.run));
    }
}
