#ifndef __TRAP_H__
#define __TRAP_H__

#define TIMER_SCAUSE 0x8000000000000005L
#define EXTERNAL_SCAUSE 0x8000000000000009L

#define E_SYSCALL 8L       // 系统调用
#define E_INS_PF 12L       // 指令缺页
#define E_LOAD_PF 13L      // 加载缺页
#define E_STORE_AMO_PF 15L // 存储缺页

extern void trap_init();
extern void trap_inithart();

#endif