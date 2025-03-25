#include "core/signal.h"

#include "core/proc.h"
#include "lib/semaphore.h"
#include "lib/spinlock.h"
#include "mm/kmalloc.h"
#include "std/stdio.h"
extern  pid_t do_waitpid(pid_t pid, int *status, int options);

static void sigint_handler(int sig)
{
    printk("sigint_default_handler: %d\n", sig);
}

static void sigquit_handler(int sig)
{
    printk("sigquit_default_handler: %d\n", sig);
}

static void sigtrap_handler(int sig)
{
    printk("sigtrap_default_handler: %d\n", sig);
}

static void sigiot_handler(int sig)
{
    printk("sigiot_default_handler: %d\n", sig);
}

static void sigfpe_handler(int sig)
{
    printk("sigfpe_default_handler: %d\n", sig);
}

static void sigkill_handler(int sig)
{
    printk("sigkill_default_handler: %d\n", sig);
}

static void sigusr1_handler(int sig)
{
    printk("sigusr1_default_handler: %d\n", sig);
}

static void sigsegv_handler(int sig)
{
    printk("sigsegv_default_handler: %d\n", sig);
}

static void sigusr2_handler(int sig)
{
    printk("sigusr2_default_handler: %d\n", sig);
}

static void sigalrm_handler(int sig)
{
    printk("sigalrm_default_handler: %d\n", sig);
}

static void sigterm_handler(int sig)
{
    printk("sigterm_default_handler: %d\n", sig);
}

static void sigchld_handler(int sig)
{
    do_waitpid(-1, NULL, 1);
}

static void sigcont_handler(int sig)
{
    printk("sigcont_default_handler: %d\n", sig);
}

static void sigstop_handler(int sig)
{
    printk("sigstop_default_handler: %d\n", sig);
}

static void sigtstp_handler(int sig)
{
    printk("sigtstp_default_handler: %d\n", sig);
}

static void (*default_signal_handlers[_NSIG])(int) = {
        [SIGINT] = sigint_handler,
        [SIGQUIT] = sigquit_handler,
        [SIGTRAP] = sigtrap_handler,
        [SIGIOT] = sigiot_handler,
        [SIGFPE] = sigfpe_handler,
        [SIGKILL] = sigkill_handler,
        [SIGUSR1] = sigusr1_handler,
        [SIGSEGV] = sigsegv_handler,
        [SIGUSR2] = sigusr2_handler,
        [SIGALRM] = sigalrm_handler,
        [SIGTERM] = sigterm_handler,
        [SIGCHLD] = sigchld_handler,
        [SIGCONT] = sigcont_handler,
        [SIGSTOP] = sigstop_handler,
        [SIGTSTP] = sigtstp_handler,
};

static void signal_struct_init(struct signal_struct *ss)
{
    spin_init(&ss->lock, "ss");
    for (int i = 0; i < _NSIG; i++) {
        ss->siga[i].sa_handler = default_signal_handlers[i];
    }
}

static void sigpending_init(struct sigpending *sping)
{
    fifo_init(&sping->queue);
    sping->signal = 0;
}

static inline int detect_sig_valid(int sig)
{
    if (sig < 0 || sig >= _NSIG) {
        printk("detect_sig_valid\n");
        return -1;
    }
    return 0;
}

// 需要 signal 锁
static int std_sig_pop(sigset_t *s)
{
    int found = -1;
    for (uint8 i = 0; i < _N_STDSIG; i++) {
        if ((*s & (1 << i)) != 0) {
            found = i;
            *s &= ~(1 << i);  // 擦除信号
            break;
        }
    }
    return found;
}

// 需要 signal 锁
static int realtime_sig_pop(struct sigpending *sping)
{
    siginfo_t *sig = list_entry(fifo_pop(&sping->queue), siginfo_t, list);
    if (!sig)
        return -1;
    int info = sig->info;
    kfree(sig);
    return info;
}

static void realtime_sig_push(int sig, struct sigpending *sping)
{
    siginfo_t *s = kmalloc(sizeof(siginfo_t), 0);
    if (!s)
        panic("realtime_sig_push\n");
    s->info = sig;
    INIT_LIST_HEAD(&s->list);
    fifo_push(&sping->queue, &s->list);
}

static int sig_pop(struct sigpending *sping)
{
    int s = std_sig_pop(&sping->signal);
    if (s != -1)
        return s;
    s = realtime_sig_pop(sping);
    return s;
}

void sig_init(struct signal *s)
{
    s->sig = kmalloc(sizeof(struct signal_struct), 0);
    if (!s->sig)
        panic("sig_init\n");

    spin_init(&s->lock, "signal");
    s->sigpend = 0;
    sigpending_init(&s->sping);
    signal_struct_init(s->sig);
}

// 此函数在从内核返回用户的时候调用
// 检测并处理信号
void signal_handler(struct signal *s)
{
    spin_lock(&s->lock);
    if (s->sigpend == 0) {
        spin_unlock(&s->lock);
        return;
    }

    int si = sig_pop(&s->sping);
    if (si == -1) {
        // 感觉很难发生这个，因为前面有对 sigpend 的检测
        spin_unlock(&s->lock);
        return;
    }
    s->sigpend--;
    spin_lock(&s->sig->lock);
    __sighandler_t do_sig_handler = s->sig->siga[si].sa_handler;
    spin_unlock(&s->sig->lock);
    if (do_sig_handler == NULL) {
        printk("do_sig_handler NULL, sig: %d\n", si);
        spin_unlock(&s->lock);
        return;
    }
    spin_unlock(&s->lock);

    do_sig_handler(si);
}

void send_sig(int sig, pid_t pid)
{
    if (detect_sig_valid(sig) < 0) {
        printk("invalid sig\n");
        return;
    }

    struct thread_info *p = find_proc(pid);
    if (!p) {
        printk("send_sig process not found\n");
        return;
    }

    if (p->state == ZOMBIE) {
        printk("send_sig process ZOMBIE\n");
        return;
    }

    struct task_struct *t = p->task;
    spin_lock(&t->sigs.lock);

    struct sigpending *sp = &t->sigs.sping;

    // 被阻塞，即 blocked 对应的位为 1
    if ((sp->blocked & (1 << sig)) != 0) {
        spin_unlock(&t->sigs.lock);
        return;
    }

    if (sig < _N_STDSIG)  // 标准信号
        sp->signal |= (1 << sig);
    else  // 实时信号
        realtime_sig_push(sig, sp);

    t->sigs.sigpend++;
    // printk("send sig: %d, pid: %d get\n", sig, p->pid);
    spin_unlock(&t->sigs.lock);
}

int sig_clear_blocked(struct signal *s, int sig)
{
    struct sigpending *sp;

    if (detect_sig_valid(sig) < 0) {
        printk("sig_clear_blocked invalid sig\n");
        return -1;
    }
    spin_lock(&s->lock);
    sp = &s->sping;
    sp->blocked &= ~(1U << sig);
    spin_unlock(&s->lock);
    return 0;
}

int sig_set_blocked(struct signal *s, int sig)
{
    struct sigpending *sp;

    if (detect_sig_valid(sig) < 0) {
        printk("sig_set_blocked invalid sig\n");
        return -1;
    }

    switch (sig) {
    case SIGSTOP:
    case SIGKILL:
        printk("sig_set_blocked: sig %d cannot be caught or ignored\n", sig);
        return -1;
    default:
        break;
    }

    spin_lock(&s->lock);
    sp = &s->sping;
    sp->blocked |= (1U << sig);
    spin_unlock(&s->lock);

    return 0;
}

void sig_refault_all(struct signal *s)
{
    struct signal_struct *ss = s->sig;
    spin_lock(&ss->lock);
    for (int i = 0; i < _NSIG; i++) {
        ss->siga[i].sa_handler = default_signal_handlers[i];
    }
    spin_unlock(&ss->lock);
}

void sig_refault(struct signal *s, int sig)
{
    if (detect_sig_valid(sig) < 0)
        return;
    struct signal_struct *ss = s->sig;
    spin_lock(&ss->lock);
    ss->siga[sig].sa_handler = default_signal_handlers[sig];
    spin_unlock(&ss->lock);
}

int sig_set_sigaction(struct signal *s, int sig, __sighandler_t handler)
{
    if (detect_sig_valid(sig) < 0) {
        printk("sig_set_sigaction invalid sig\n");
        return -1;
    }

    switch (sig) {
    case SIGSTOP:
    case SIGKILL:
        printk("sig_set_sigaction: sig %d cannot set handler\n", sig);
        return -1;
    default:
        break;
    }

    struct signal_struct *ss = s->sig;
    spin_lock(&ss->lock);
    ss->siga[sig].sa_handler = handler;
    spin_unlock(&ss->lock);
    return 0;
}

int64 do_sigaction(int sig, __sighandler_t handler)
{
    struct task_struct *t = myproc()->task;
    return sig_set_sigaction(&t->sigs, sig, handler);
}

// 在回收进程使用，没有并发冲突
void sig_release_all(struct signal *s)
{
    struct sigpending *sping = &s->sping;
    while (!fifo_empty(&sping->queue))
        realtime_sig_pop(sping);
    kfree(s->sig);
}
