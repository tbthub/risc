#ifndef __SIGNAL_H__
#define __SIGNAL_H__
#include "lib/list.h"
#include "core/locks/spinlock.h"
#include "lib/fifo.h"

#define _NSIG 64
#define _N_STDSIG 32
typedef uint32_t sigset_t;

// 中断信号，通常由 Ctrl+C 触发
#define SIGINT 2 // Interrupt from keyboard (Ctrl+C)
// 系统终止信号
#define SIGQUIT 3 // Quit signal (Ctrl+\)
// 调试信号
#define SIGTRAP 5 // Trap signal (for debugging)
// 系统终止
#define SIGIOT 6 // I/O trapped by kernel
// 异常退出信号（非法指令、数学异常等）
#define SIGFPE 8 // Floating-point exception (divide by zero, overflow, etc.)
// 强制终止信号
#define SIGKILL 9 // Kill signal (cannot be caught or ignored)
// 用户自定义信号 1
#define SIGUSR1 10 // User-defined signal 
// 段错误信号，通常由访问无效内存地址触发
#define SIGSEGV 11 // Segmentation fault
// 用户自定义信号 2
#define SIGUSR2 12 // User-defined signal 2
// 定时器到期信号
#define SIGALRM 14 // Alarm clock signal
// 终止信号，通常由 kill 命令发出
#define SIGTERM 15 // Termination signal
// 子进程状态改变时发送的信号
#define SIGCHLD 17 // Child process terminated or stopped
// 继续执行信号
#define SIGCONT 18 // Continue execution if stopped
// 停止信号，用于进程暂停
#define SIGSTOP 19 // Stop signal (cannot be caught or ignored)
#define SIGTSTP 20 // Terminal stop signal (Ctrl+Z)


// 信号处理函数类型（修正参数类型）
typedef void (*__sighandler_t)(int);

// 信号处理配置结构体
struct sigaction {
	__sighandler_t sa_handler; 
};


// 信号管理结构体
struct signal_struct {
    spinlock_t lock;
	struct sigaction siga[_NSIG];
};

typedef struct {
	struct list_head list;
	uint8 info;
} siginfo_t;



struct sigpending {
	struct fifo queue; // 实时信号
	sigset_t signal; // 标准信号
	sigset_t blocked;
};

struct signal {
	spinlock_t lock; // 用于处理信号标准合并导致的 sigpend 数量不对的问题
	uint32_t sigpend;
    struct sigpending sping;
	struct signal_struct *sig;
};

extern void sig_init(struct signal *s);
extern void send_sig(int sig, pid_t pid);
extern int sig_set_blocked(struct signal *s, int sig);
extern int sig_clear_blocked(struct signal *s, int sig);

extern void sig_refault(struct signal *s, int sig);
extern void sig_refault_all(struct signal *s);
extern int sig_set_sigaction(struct signal *s, int sig, __sighandler_t handler);


#endif
