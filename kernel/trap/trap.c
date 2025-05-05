#include "core/trap.h"

#include "core/proc.h"
#include "core/sched.h"
#include "core/timer.h"
#include "dev/plic.h"
#include "dev/uart.h"
#include "elf.h"
#include "fs/file.h"
#include "core/locks/semaphore.h"
#include "lib/string.h"
#include "mm/memlayout.h"
#include "mm/page.h"
#include "riscv.h"
#include "std/stdio.h"
#include "defs.h"
#include "elf.h"
extern void* _start;

extern void virtio_disk_intr();
// in kernelvec.S, calls kerneltrap().
extern void kernelvec();
extern void uservec();
extern void userret() __attribute__((noreturn));
extern void syscall();
extern int alloc_user_stack(struct mm_struct *mm, tid_t tid);
extern int parse_elf_header(struct elf64_hdr *ehdr, struct thread_info *t, struct file *f);
extern void page_fault_handler(uint64_t fault_addr, uint64_t scause);
extern uint64_t parse_argv(struct thread_info *t, char *const argv[], char *args_page[]);
extern void signal_handler(struct signal *s);
extern int mappages(pagetable_t pagetable, uint64_t va, uint64_t pa, uint64_t size, int perm);

void trap_init()
{
    time_init();
}

// 配置内核的异常和中断向量

// 设置异常和中断的处理程序入口，具体是将 kernelvec 函数的地址写入到 stvec
// 寄存器中。 stvec 是用于在 RISC-V 中存储异常/中断向量表的寄存器。 在 RISC-V
// 中，当发生异常或中断时，CPU 会自动将控制权转交给 stvec 中存储的地址
void trap_inithart(void)
{
    w_stvec((uint64_t)kernelvec);
}

// 时钟中断，由 intr_handler 调用
static void timer_intr()
{
    time_update();

    struct thread_info *cur = myproc();
    if (!cur) {
        // 当前没有进程，正在开中断的情况下等待
        w_stimecmp(r_time() + 8000);
        return;
    }

    // ticks 为线程的私有变量，不需要加锁
    cur->ticks--;

    w_stimecmp(r_time() + 30000);

    // 到期就让出 CPU
    if (cur->ticks == 0) {
        yield();
    }
}

// 外部中断，由 intr_handler 调用
static inline void external_intr()
{
    int irq = plic_claim();
    switch (irq) {
    case UART0_IRQ:
        uartintr();
        break;
    case VIRTIO0_IRQ:
        virtio_disk_intr();
        break;
    case 0:
        break;
    default:
        printk("unexpected interrupt irq=%d\n", irq);
        break;
    }

    if (irq)
        plic_complete(irq);
}

// 中断
static inline void intr_handler(uint64_t scause)
{
    switch (scause) {
    case EXTERNAL_SCAUSE:  // 外设
        external_intr();
        break;

    case TIMER_SCAUSE:  // 时钟
        timer_intr();
        break;

    default:
        break;
    }
}
extern void __attribute__((unused)) vm2pa_show(struct mm_struct *mm);

extern void show_all_args(struct thread_info *t);
// 异常
static void excep_handler(uint64_t scause)
{
    switch (scause) {
    case E_SYSCALL:
        syscall();
        break;

    case E_INS_PF:
    case E_LOAD_PF:
    case E_STORE_AMO_PF:
        page_fault_handler(r_stval(), scause);
        break;
        
    default:
        struct thread_info *t = myproc();
        printk("pid: %d, unknown scause\n", t->pid);
        if (t)
            printk("pid: %d, unknown scause: %p, sepc: %p, stval: %p\n", t->pid, scause, r_sepc(), r_stval());
        else
            printk("kerl, unknown scause: %p, sepc: %p, stval: %p\n", t->pid, scause, r_sepc(), r_stval());
        printk("satp: %p\n", r_satp());
        printk("proc satp: %p\n", myproc()->task->mm.pgd);
        printk("page: %d\n", page_count(PA2PG(myproc()->task->mm.pgd)));
        printk("intr noff: %d\n",cpus[cpuid()].noff);
        show_all_args(t);
        panic("excep_handler unknown scause, sp:%p\n", t->tf->sp);
        panic("\n");
        break;
    }
}

__attribute__((noreturn)) void usertrapret()
{
    intr_off();
    struct thread_info *p = myproc();
    signal_handler(&p->task->sigs);
    // 我们即将把陷阱的目标从
    // kerneltrap() 切换到 usertrap()，因此请关闭中断，直到
    // 我们回到用户空间，此ma时 usertrap() 是正确的。(系统调用之前会开中断)

    spin_lock(&p->task->mm.lock);
    // 如果发生了进程切换
    if (r_satp() != MAKE_SATP(p->task->mm.pgd)) {
        sfence_vma();
        w_satp(MAKE_SATP(p->task->mm.pgd));
        sfence_vma();
    }
    spin_unlock(&p->task->mm.lock);

    w_stvec((uint64_t)uservec);
    w_sepc(p->tf->epc);
    w_sscratch((uint64_t)(p->tf));

    // set S Previous Privilege mode to User.
    unsigned long x = r_sstatus();
    x &= ~SSTATUS_SPP;  // clear SPP to 0 for user mode
    x |= SSTATUS_SPIE;  // enable interrupts in user mode
    w_sstatus(x);
    userret();
}

// exec 会使用原来的内核栈
// PS 这个函数确实写的太丑，不过我们很懒，能跑就行
__attribute__((noreturn)) int do_exec(const char *path, char *const argv[])
{
    int res = -1;

    char *args_page[USER_ARGV_MAX_SIZE];
    // * 用户传入的 path 可能没有被映射如内存，最好确保访问一次
    if (!(path && READ_ONCE(*path)))
        ;

    if (!(argv && READ_ONCE(*argv)))
        ;

    // assert(intr_get() == 0,"exec intr 1\n");
    struct thread_info *t = myproc();
#ifdef DEBUG_SYSCALL
    printk("[exec] pid: %d\n", t->pid);
#endif
    strncpy(t->name, path, sizeof(t->name));
    assert(intr_get() == 0, "exec intr 2\n");

    struct elf64_hdr ehdr;
    struct file *f = file_open(path, FILE_READ);
    assert(intr_get() == 0, "exec intr 3\n");

    file_lock(f);
    if (!f)
        panic("do_exec f is NULL\n");

    file_read(f, &ehdr, sizeof(ehdr));

    assert(intr_get() == 0, "exec intr 4\n");

    struct mm_struct *mm = &t->task->mm;

    // ! 解析参数的时候，前面已经 free_user_memory

    uint64_t args_list;
    if (argv) {
        args_list = parse_argv(t, argv, args_page);
        if (args_list == 0)
            panic("do_exec parse_argv\n");
    }
    assert(intr_get() == 0, "exec xxxxxxxxn\n");

    if (mm->pgd)
        free_user_memory(mm);

    alloc_user_pgd(mm);

    if (argv) {
        struct vm_area_struct *v = vma_alloc_proghdr(USER_ARGS_PAGE, USER_ARGS_PAGE + (USER_ARGS_MAX_CNT + USER_ARGV_MAX_SIZE) * PGSIZE - 1,
         ELF_PROG_FLAG_READ | ELF_PROG_FLAG_WRITE, 0, NULL, &vma_args_ops);
        vma_insert(mm, v);

        mappages(mm->pgd, PGROUNDDOWN(USER_ARGS_PAGE), args_list, PGSIZE, PTE_R | PTE_W | PTE_U);

        for (int i = 0; i < USER_ARGV_MAX_SIZE; i++)
            mappages(mm->pgd, PGROUNDDOWN(USER_ARGS_PAGE + (USER_ARGS_MAX_CNT + i) * PGSIZE), (uint64_t)args_page[i], PGSIZE, PTE_R | PTE_W | PTE_U);
    }

    res = alloc_user_stack(&t->task->mm, t->tid);
    if (res == -1)
        panic("do_exec alloc_user_stack\n");

    parse_elf_header(&ehdr, t, f);

    file_unlock(f);
    // vma_list_cat(mm->mmap);

    sig_refault_all(&t->task->sigs);

    memset(t->tf,0,sizeof(*t->tf));
    t->tf->epc = (uint64_t)user_entry;
    t->tf->a0 = ehdr.entry;
    t->tf->sp = USER_STACK_TOP(t->tid);

    t->tf->kernel_sp = KERNEL_STACK_TOP(t);
#ifdef DEBUG_SYSCALL
    printk("[exec] end\n");
#endif
    usertrapret();
}

// 内核 trap 处理函数 kernel_trap
void kerneltrap()
{
    uint64_t sepc = r_sepc();        // sepc: 保存异常发生时的 sepc 寄存器的值，sepc
                                   // 存储了引发异常或中断时的程序计数器（PC）的值
    uint64_t sstatus = r_sstatus();  // sstatus: 保存当前的 sstatus 寄存器的值，sstatus
                                   // 包含状态标志，例如当前的运行模式
    uint64_t scause = r_scause();    // scause: 保存 scause 寄存器的值，scause
    assert((sstatus & SSTATUS_SPP) != 0, "kerneltrap: not from supervisor mode %d", sstatus & SSTATUS_SPP);
    assert(intr_get() == 0,
           "kerneltrap: interrupts enabled");  // xv6不允许嵌套中断，因此执行到这里的时候一定是关中断的

    if (scause & (1ULL << 63))
        intr_handler(scause);
    else {
        excep_handler(scause);
    }

    w_sepc(sepc);        // 将原来的 sepc
                         // 值写回寄存器，以便异常返回时能够继续原来的代码执行。
    w_sstatus(sstatus);  //  恢复 sstatus 的状态，以确保内核的状态和之前一致。
}

// 用户 trap 处理函数 user_trap
void usertrap()
{
    uint64_t scause = r_scause();
    uint64_t sstatus = r_sstatus();
    uint64_t sepc = r_sepc();
    // printk("[u-trap] pid: %d, scause: %p\n",myproc()->pid,scause);
    // 检查是否来自用户模式下的中断，也就是不是内核中断,确保中断来自用户态
    assert((sstatus & SSTATUS_SPP) == 0, "usertrap: not from user mode\n");
    assert(intr_get() == 0,
           "usertrap: interrupts enabled");  // xv6不允许嵌套中断，因此执行到这里的时候一定是关中断的

    // 现在位于内核，要设置 stvec 为 kernelvec
    w_stvec((uint64_t)kernelvec);

    // (位于进程上下文的，别忘记了:-)
    struct thread_info *p = myproc();
    assert(p != NULL, "usertrap: p is NULL\n");
    p->tf->epc = sepc;

    if (scause & (1ULL << 63))
        intr_handler(scause);
    else {
        // 额外处理下如果是系统调用
        if (scause == E_SYSCALL)
            // 返回到系统调用的下一条指令，即越过 ecall
            p->tf->epc += 4;
        excep_handler(scause);
    }

    usertrapret();
}
