#ifndef __CONF_H__
#define __CONF_H__

#define DEBUGS
#ifdef DEBUGS

// #define DEBUG_TEST         // 用于测试程序
// #define DEBUG_ORD          // 普通调试信息
// #define DEBUG_FS           // 初始显示文件系统信息
// #define DEBUG_FLUSH        // 显示 flush 线程的刷写信息
// #define DEBUG_EFS_SYNC     // 显示 efs_sync 同步 sb,imap,bmap,inode的信息
// #define DEBUG_BIO          // 显示 bio 请求链
// #define DEBUG_GEN_BUF      // 显示 gendisk buf 读写情况
// #define DEBUG_TASK_ADD_CPU // 显示任务添加到所在的 cpuid
// #define DEBUG_TASK_ON_CPU  // 显示当前 CPU 上运行的线程名
// #define DEBUG_RQ           // 显示 IO 请求
// #define DEBUG_COW          // 显示 COW
// #define DEBUG_PF           // 显示缺页
#define DEBUG_SF_PFMAP     // 显示栈、文件页面映射（因缺页）
// #define DEBUG_SYSCALL      // 显示来自用户空间的系统调用
#define DEBUG_EXIT         // 显示线程退出情况
#define DEBUG_WAIT         // 显示 wait 回收情况

#endif
// TODO
#define MAX_PATH_LEN 256 // 支持最长路径长度
#define ARG_MAX 128 * 1024 // 传入参数最长 128K
#define ENV_MAX 128 * 1024 // 环境变量最长 128K

#define USER_TEXT_BASE 0x200000000 // 用户程序代码段基地址 0x200_000_000
#define USER_STACK_SIZE (8 * 1024 * 1024) // 8 MB 栈
#define USER_STACK_TOP(tid) (0x200000000 - 0x1000 - (tid) * (USER_STACK_SIZE + 0x1000)- 0x8) // 用户程序栈顶

#define USER_TEXT_BASE 0x200000000 // 用户程序代码段基地址 0x200_000_000

// 用户参数页列表占用 1 页(最多 4096 / 8 = 512 个参数，显示包含最后一个 NULL)
// (暂时固定，要改的话需要该代码 parse_argv )
#define USER_ARGS_MAX_CNT 1

#define USER_ARGV_MAX_SIZE 4 // 用户参数页大小 4 个页
#define USER_ARGV_MAX_CNT (PGSIZE / sizeof(char *)) // 最大参数个数
#define USER_ARGS_PAGE 0x150000000

#define USER_MAP_TOP 0xFFFFFFFFF // 用户映射区顶
#define USER_MAP_BOTTOM 0xa00000000 // 用户映射区域底

#define MAX_THREADS_PER_TASK 64

#endif
