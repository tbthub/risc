#include "core/proc.h"

#include "core/sched.h"
#include "core/signal.h"
#include "core/vm.h"
#include "error.h"
#include "lib/hash.h"
#include "lib/list.h"
#include "lib/string.h"
#include "mm/kmalloc.h"
#include "mm/mm.h"
#include "mm/slab.h"
#include "riscv.h"
#include "std/stdarg.h"

static struct
{
    // 用于分配 pid 由于我们使用了slab回收的可以直接利用
    int pids;
    // 保护pids的递增
    spinlock_t lock;

    struct hash_table global_proc_table;

} Proc;

struct cpu cpus[NCPU];
struct thread_info *init_t;

extern void usertrapret() __attribute__((noreturn));
extern int vm_stack_load(struct thread_info *t, struct vm_area_struct *v, uint64 fault_addr);
extern int mappages(pagetable_t pagetable, uint64 va, uint64 pa, uint64 size, int perm);
extern void files_init(struct files_struct *files);
extern void sig_release_all(struct signal *s);
__attribute__((noreturn)) int64 do_exit(int exit_code);

struct thread_info *get_init()
{
    return init_t;
}

// 退出（ZOMBIE）后重新调度
static void quit()
{
    struct cpu *cpu = mycpu();
    struct thread_info *thread = cpu->thread;

    spin_lock(&cpu->sched_list.lock);
    list_add_tail(&thread->sched, &cpu->sched_list.out);
    spin_unlock(&cpu->sched_list.lock);

    do_exit(0);

    sched();
}

// 线程出口函数，由 thread_entry 执行完 func 后调用
static void thread_exit()
{
    quit();
}

// 线程入口函数，用于执行传入的线程函数
static void thread_entry()
{
    struct thread_info *thread = myproc();
    // 第一次需要释放锁
    // printk("Thread %s release lock on cpu %d in thread_entry  --only
    // once\n", thread->name, cpuid());
    spin_unlock(&thread->lock);
    intr_on();  // 需要开中断。由时钟中断->调度器切换->是关了中断的，要重新打开
    thread->func(thread->args);
    thread_exit();
}

static inline void mm_init(struct mm_struct *mm)
{
    memset(mm, 0, sizeof(struct mm_struct));
    spin_init(&mm->lock, "mm_struct");
    mm->next_map = USER_MAP_TOP;
}

// static int th_table_init(struct th_table_struct *th_table)
// {
//     th_table->array = kmalloc(sizeof(struct thread_info *) * MAX_THREADS_PER_TASK, 0);
//     if (!th_table->array) {
//         panic("th_table_init\n");
//         return -ENOMEM;
//     }
//     spin_init(&th_table->lock, "th_table");
//     sem_init(&th_table->main_thread_wait, 0, "th_table");
//     th_table->num = 1;
//     th_table->alloc = 0;
//     return 0;
// }

// static void th_table_free(struct th_table_struct *th_table)
// {
//     if (!th_table->array)
//         return;

//     kfree(th_table->array);
// }

// TODO 后面支持多线程需要
// static int th_table_alloc(struct th_table_struct *th_table)
// {
// 	spin_lock(&th_table->lock);
// 	// 如果已经满了
// 	if (th_table->alloc == MAX_THREADS_PER_TASK) {
// 		return -1;
// 	}
// 	int found = -1;
// 	for (uint32 i = th_table->alloc; i < MAX_THREADS_PER_TASK; i++) {
// 		if (th_table->array[i] == NULL) {
// 			found = i;
// 			th_table->alloc = i + 1;
// 			break;
// 		}
// 	}
// 	spin_unlock(&th_table->lock);
// 	return found;
// }

// static void th_table_free(struct th_table_struct *th_table, int index)
// {
// 	assert(th_table != NULL, "th_table_free th_table");
// 	assert(th_table->array != NULL, "th_table_free array");
// 	assert(index > -1 && index < MAX_THREADS_PER_TASK,
// 	       "th_table_free index\n");

// 	spin_lock(&th_table->lock);
// 	th_table->array[index] == NULL;
// 	th_table->alloc = index;
// 	spin_unlock(&th_table->lock);
// }

// 初始化 task_struct
static inline void task_struct_init(struct task_struct *task)
{
    spin_init(&task->lock, "task");
    mm_init(&task->mm);
    sig_init(&task->sigs);
    files_init(&task->files);
    // th_table_init(&task->th_table);
}

static inline void thread_info_init(struct thread_info *thread)
{
    thread->task = NULL;
    spin_init(&thread->lock, "thread");
    thread->flags = 0;
    thread->state = USED;
    thread->tid = 0;

    thread->parent = NULL;
    INIT_LIST_HEAD(&thread->child);
    INIT_LIST_HEAD(&thread->sibling);
    INIT_LIST_HEAD(&thread->sched);
    INIT_HASH_NODE(&thread->global);

    sem_init(&thread->child_exit_sem, 0, "child_exit");

    thread->tf = NULL;
    thread->ticks = 10;
    thread->args = NULL;
    thread->func = NULL;
    thread->cpu_affinity = NO_CPU_AFF;

    memset(&thread->context, 0, sizeof(thread->context));

    thread->context.sp = KERNEL_STACK_TOP(thread);
}

// 申请一个空白的PCB，包括 thread_info + task_struct
static struct thread_info *alloc_thread()
{
    struct task_struct *task = kmem_cache_alloc(&task_struct_kmem_cache);
    if (!task)
        return NULL;
    struct thread_info *thread = kmem_cache_alloc(&thread_info_kmem_cache);
    if (!thread) {
        kmem_cache_free(&task_struct_kmem_cache, task);
        return NULL;
    }
    task_struct_init(task);
    thread_info_init(thread);

    spin_init(&thread->lock, "thread");

    // 不会并发冲突
    thread->task = task;
    thread->tid = 0;
    // task->th_table.array[0] = thread;
    // task->th_table.alloc = 1;

    spin_lock(&Proc.lock);
    thread->pid = thread->pid == 0 ? Proc.pids++ : thread->pid;
    hash_add_head(&Proc.global_proc_table, thread->pid, &thread->global);
    spin_unlock(&Proc.lock);

    return thread;
}

// 这里应该是从 scheduler 的 swtch 进入
// 在 scheduler 中获取了锁，这里需要释放
static void forkret()
{
    // printk("forkret, pid: %d",myproc()->pid);
    spin_unlock(&myproc()->lock);
    usertrapret();
}

struct thread_info *kthread_struct_init()
{
    struct thread_info *t = alloc_thread();
    if (!t) {
        printk("kthread_struct_init error!\n");
        return NULL;
    }
    t->context.ra = (uint64)thread_entry;
    return t;
}

struct thread_info *uthread_struct_init()
{
    struct thread_info *t = alloc_thread();
    if (!t) {
        printk("uthread_struct_init error!\n");
        return NULL;
    }
    t->tf = kmem_cache_alloc(&tf_kmem_cache);
    if (!t->tf) {
        panic("uthread_struct_init tf_kmem_cache\n");
        // printk("uthread_struct_init tf_kmem_cache\n");
        // TODO 添加回收的逻辑，不过我们暂时先这样，不考虑异常，直接堵死
    }
    t->context.ra = (uint64)forkret;
    return t;
}

// 获取 cpuid
// * 必须在关中断环境下,防止与进程移动发生竞争，到不同的CPU。
inline int cpuid()
{
    return r_tp();
}

// 返回该 CPU 的 cpu 结构体。
// * 必须在关中断环境下。
inline struct cpu *mycpu()
{
    int id = cpuid();
    struct cpu *c = &cpus[id];
    return c;
}

struct thread_info *myproc()
{
    push_off();
    struct thread_info *thread = mycpu()->thread;
    pop_off();
    return thread;
}

void proc_init()
{
    spin_init(&Proc.lock, "Proc");
    Proc.pids = 0;
    hash_init(&Proc.global_proc_table, 41, "Proc hash");
}

struct thread_info *find_proc(int _pid)
{
    struct thread_info *t;
    hash_find(t, &Proc.global_proc_table, pid, _pid, global);
    return t;
}

// 以后我们添加环境变量参数，这个应该是共享映射
uint64 parse_argv(struct thread_info *t, char *const argv[], char *args_page[])
{
    assert(USER_ARGV_MAX_SIZE > 0, "parse_argv USER_ARGV_MAX_SIZE");
    // 申请页面，我们将第一个页面用于参数的 char *
    // 指针，大概最多可支持 4096/8 = 512个参数
    // 其后面的页面用于容纳参数的内容。
    char **args_list = NULL;
    args_list = __alloc_page(0);
    if (!(args_list))
        goto bad;

    for (int i = 0; i < USER_ARGV_MAX_SIZE; i++) {
        args_page[i] = __alloc_page(0);
        if (args_page[i] == NULL)
            goto bad;
    }

    int page_index = 0;
    char *args = args_page[page_index];
    int argc = 0;
    int args_len = 0;
    int arg_max_len = USER_ARGV_MAX_SIZE * PGSIZE;

    for (char *const *arg = argv; *arg != NULL; arg++) {
        uint32 len = strlen(*arg) + 1;

        if (args_len + len > arg_max_len || argc >= USER_ARGV_MAX_CNT) {
            printk("Arguments too long or too many arguments");
            goto bad;
        }

        // 记录当前参数起始位置
        args_list[argc] = args;

        // 如果当前页空间不足
        if (PGSIZE - ((uint64)args % PGSIZE) < len) {
            uint32 part_len = PGSIZE - ((uint64)args % PGSIZE);
            memcpy(args, *arg, part_len);
            args_len += part_len;

            args = args_page[++page_index];
            memcpy(args, *arg + part_len, len - part_len);
            args += len - part_len;
        }
        else {
            memcpy(args, *arg, len);
            args += len;
        }

        args_len += len;
        argc++;
    }

    // ! riscv64 是通过寄存器传参的
    t->tf->a0 = argc;
    t->tf->a1 = (uint64)args_list;
    return (uint64)args_list;

bad:
    for (int i = 0; i < USER_ARGV_MAX_SIZE; i++) {
        if (args_page[i])
            __free_page(args_page[i]);
    }
    if (args_list)
        __free_page(args_list);
    return 0;
}

/*
 *  fd ops
 *  下面的函数都需要有 file_lock
 */

fd_t get_unused_fd(struct files_struct *files)
{
    for (int i = 0; i < NOFILE; i++) {
        if (files->fdt[i] == NULL)
            return i;
    }
    return -EMFILE;
}

void fd_install(struct files_struct *files, fd_t fd, struct file *f)
{
    assert(f != NULL, "fd_install");
    assert(fd > -1 && fd < NOFILE, "fd_install fd");
    files->fdt[fd] = f;
}

void fd_close(struct files_struct *files, fd_t fd)
{
    struct file **f;
    if (!files)
        return;
    assert(fd > -1 && fd < NOFILE, "fd_close fd");

    f = files->fdt + fd;
    if (*f != NULL) {
        file_close(*f);
        *f = NULL;
    }
}

void fd_close_all(struct files_struct *files)
{
    struct file **f;
    if (!files)
        return;

    for (int i = 0; i < NOFILE; i++) {
        f = files->fdt + i;
        if (*f != NULL) {
            file_close(*f);
            *f = NULL;
        }
    }
}

/*
 * fork
 * 复制
 *  - thread_info:
 *      - 上下文
 *  - task_struct：
 *      - 虚存、已映射页表
 *      - 文件
 *      - 信号
 */

static void copy_context(struct thread_info *ch, struct thread_info *pa)
{
    *ch->tf = *pa->tf;
    // 子进程返回的 pid 为 0
    // ch->tf->a0 = 0;
}

static void copy_vm(struct task_struct *ch, struct task_struct *pa)
{
    // TODO 这里其实。。。有一点小问题，子进程的 vma 变成倒序了
    struct mm_struct *pa_mm = &pa->mm;
    struct mm_struct *ch_mm = &ch->mm;

    alloc_user_pgd(ch_mm);

    struct vm_area_struct *pa_vma = pa_mm->mmap;
    while (pa_vma) {
        pa_vma->vm_ops->dup(pa_mm, pa_vma, ch_mm);
        pa_vma = pa_vma->vm_next;
    }
}

static void copy_files(struct task_struct *ch, struct task_struct *pa)
{
    struct files_struct *pa_files = &ch->files;
    struct files_struct *ch_files = &pa->files;

    spin_lock(&pa_files->file_lock);
    struct file **pa_fdt = pa_files->fdt;
    struct file **ch_fdt = ch_files->fdt;

    for (int i = 0; i < NOFILE; i++, pa_fdt++, ch_fdt++) {
        if (*pa_fdt)
            file_dup(*pa_fdt);
        *ch_fdt = *pa_fdt;
    }
    spin_unlock(&pa_files->file_lock);
}

static void copy_sigs(struct task_struct *ch, struct task_struct *pa)
{
    struct signal *ch_sigs = &ch->sigs;
    struct signal *pa_sigs = &pa->sigs;

    spin_lock(&pa_sigs->lock);
    ch_sigs->sigpend = pa_sigs->sigpend;
    *ch_sigs->sig = *pa_sigs->sig;

    ch_sigs->sping.blocked = pa_sigs->sping.blocked;
    ch_sigs->sping.signal = pa_sigs->sping.signal;
    spin_unlock(&pa_sigs->lock);
}

static void copy_proc(struct thread_info *ch, struct thread_info *pa)
{
    struct task_struct *ch_task = ch->task;
    struct task_struct *pa_task = pa->task;

    copy_context(ch, pa);
    copy_vm(ch_task, pa_task);
    copy_files(ch_task, pa_task);
    copy_sigs(ch_task, pa_task);
}

void __attribute__((unused)) vm2pa_show(struct mm_struct *mm);
int do_fork()
{
    struct thread_info *pa = myproc();

    struct thread_info *ch = uthread_struct_init();
    if (!ch)
        panic("do_fork uthread_struct_init\n");

    printk("[fork] pa-pid: %d, child-pid: %d\n", pa->pid, ch->pid);

    copy_proc(ch, pa);

    ch->parent = pa;
    spin_lock(&pa->lock);
    list_add_head(&ch->sibling, &pa->child);
    spin_unlock(&pa->lock);

    // vma_list_cat(ch->task->mm.mmap);
    ch->tf->kernel_sp = KERNEL_STACK_TOP(ch);
    ch->tf->a0 = 0;
    strdup(ch->name, pa->name);
    wakeup_process(ch);
    printk("[fork] end\n");
    return ch->pid;
}

pid_t do_getpid()
{
    struct thread_info *t = myproc();
    printk("get pid: %d\n", t->pid);
    return t->pid;
}

#define WNOHANG 0x00000001  // 立即返回，不阻塞

// 找到一个子进程僵尸，返回 pcb 指针
static struct thread_info *__waitpid(pid_t pid, int *status, int options)
{
    struct thread_info *cur = myproc();
    struct thread_info *t;
    struct thread_info *tmp;

    while (1) {
        spin_lock(&cur->lock);
        list_for_each_entry_safe(t, tmp, &cur->child, sibling)
        {
            spin_lock(&t->lock);
            if (t->state == ZOMBIE) {
                if (pid == -1) {  // 寻找任意子线程
                    list_del(&t->sibling);
                    spin_unlock(&t->lock);
                    spin_unlock(&cur->lock);
                    return t;
                }
                else if (pid > 0 && t->pid == pid) {  // 等待进程 ID 为 pid 的特定子进程。
                    list_del(&t->sibling);
                    spin_unlock(&t->lock);
                    spin_unlock(&cur->lock);
                    return t;
                }
            }
            spin_unlock(&t->lock);
        }

        spin_unlock(&cur->lock);
        // 没有找到
        if (options == WNOHANG)  // 不阻塞
            return NULL;
        else  // 堵塞,继续while循环获取锁
        {
            sem_wait(&cur->child_exit_sem);
        }
    }
}

// 把所有孩子移交给 init_t
// TODO 子线程退出，主线程负责回收，要将其孩子放给 init
static void reparent(struct thread_info *t)
{
    struct thread_info *cur = myproc();

    spin_lock(&init_t->lock);
    spin_lock(&cur->lock);

    if (list_len(&cur->child) != 0) {
        printk("reparent\n");
        list_splice_head(&cur->child, &init_t->child);
    }

    spin_unlock(&cur->lock);
    spin_unlock(&init_t->lock);
}

__attribute__((noreturn)) int64 do_exit(int exit_code)
{
    // (exit) 向所有线程发送 SIGKILL，置 th_table 位
    // (exit) 主线程回收子线程的所有资源（主要是kmalloc这些）
    // -- (所有子线程已死) --
    // (exit) 释放 th_table_array
    // (exit) 关闭打开的文件
    // (exit) 移交自己孩子
    // (exit) 释放所有没处理的实时信号队列 + signal_struct
    // (exit) 释放用户地址空间
    // (exit) 回收 trapframe，如果有的话
    // (exit) 唤醒父进程的 child_exit_sem
    // (exit) 进入调度器，且不应该再回来

    // TODO 发给所有子线程并睡眠在上
    struct thread_info *t = myproc();
    printk("[exit], pid: %d\n", t->pid);
    if (t == init_t) {
        panic("init_t try to exit!\n");
    }

    struct task_struct *task = t->task;
    // 主线程
    if (t->tid == 0) {
        // th_table_free(&task->th_table);

        fd_close_all(&task->files);

        reparent(t);

        sig_release_all(&task->sigs);

        free_user_memory(&task->mm);

        if (t->tf)
            kmem_cache_free(&tf_kmem_cache, t->tf);

        // send_sig(SIGCHLD, t->parent->pid);
        // printk("pid: %d, send sig ->pa: %d\n", t->pid, t->parent->pid);

        spin_lock(&t->lock);
        t->state = ZOMBIE;
        t->exit_code = exit_code;

        sem_signal(&t->parent->child_exit_sem);
#ifdef DEBUG_EXIT
        printk("[exit] end pid: %d, thread: %s exit\n", t->pid, t->name);
#endif
        
        sched();
        // 不会在回来了
        panic("exit ret\n");
        __builtin_unreachable();
    }
    else {
        panic("exit ret2\n");
        __builtin_unreachable();
    }
}

pid_t do_waitpid(pid_t pid, int *status, int options)
{
    pid_t _pid;
    struct thread_info *ch = __waitpid(pid, status, options);
    if (ch == NULL)
        return -1;
    intr_off();
    // assert(list_empty(&ch->sched), "do_waitpid sched\n");
    // (wait) 释放顶层页表
    free_user_pgd(&ch->task->mm);

    // 移除各种结构（sibling(在__waitpid已移除)，global，task,thread_info）
    kmem_cache_free(&task_struct_kmem_cache, ch->task);

    spin_lock(&Proc.lock);
    hash_del_node(&Proc.global_proc_table, &ch->global);
    spin_unlock(&Proc.lock);
    if (status)
        *status = ch->exit_code;
    _pid = ch->pid;

#ifdef DEBUG_WAIT
    printk("[wait]: pid: %d, thread: %s code:%d ok. by %d\n", ch->pid, ch->name, ch->exit_code,ch->parent->pid);
#endif
    kmem_cache_free(&thread_info_kmem_cache, ch);

    return _pid;
}
