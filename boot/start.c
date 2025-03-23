#include "std/stddef.h"
#include "param.h"
#include "mm/memlayout.h"
#include "riscv.h"

void main();
void timerinit();

// entry.S needs one stack per CPU.
__attribute__((aligned(16))) char stack0[4096 * NCPU];

// entry.S jumps here in machine mode on stack0.
void start()
{
  // set M Previous Privilege mode to Supervisor, for mret.
  // CPU 从机器模式切换到管理模式
  unsigned long x = r_mstatus();
  x &= ~MSTATUS_MPP_MASK;
  x |= MSTATUS_MPP_S;
  w_mstatus(x);

  // set M Exception Program Counter to main, for mret.
  // requires gcc -mcmodel=medany
  // 当执行 mret 指令时，CPU 将跳转到 mepc 中保存的地址。因此，这里将 mepc 设置为操作系统内核的入口函数 main()。
  w_mepc((uint64)main);

  // 禁用分页
  w_satp(0);

  // 委托中断和异常处理给S-mode模式
  w_medeleg(0xffff);
  w_mideleg(0xffff);
  // 外设、时钟、软
  w_sie(r_sie() | SIE_SEIE | SIE_STIE | SIE_SSIE);

  // configure Physical Memory Protection to give supervisor mode
  // access to all of physical memory.
  w_pmpaddr0(0x3fffffffffffffull);
  w_pmpcfg0(0xf); // 访问权限为 0xf，即允许管理模式和用户模式访问所有内存。

  // ask for clock interrupts.
  timerinit();

  // keep each CPU's hartid in its tp register, for cpuid().
  int id = r_mhartid();
  w_tp(id);

  // switch to supervisor mode and jump to main().
  asm volatile("mret");
}

// ask each hart to generate timer interrupts.
void timerinit()
{
  // enable supervisor-mode timer interrupts.
  w_mie(r_mie() | MIE_STIE);

  // enable the sstc extension (i.e. stimecmp).
  w_menvcfg(r_menvcfg() | (1L << 63));

  // allow supervisor to use stimecmp and time.
  w_mcounteren(r_mcounteren() | 2);

  // ask for the very first timer interrupt.
  // 设置时钟中断发生的间隙，即 1000_000 个节拍发生一次时钟中断
  w_stimecmp(r_time() + 1000000);
}
