#include "std/stddef.h"
#include "mm/memlayout.h"
#include "std/stddef.h"
#include "core/proc.h"

//
// the riscv Platform Level Interrupt Controller (PLIC).
// 管理外部中断的硬件模块（类似于x86下的8259A）
//

void plic_init(void)
{
  // set desired IRQ priorities non-zero (otherwise disabled).
  // UART0_IRQ 和 VIRTIO0_IRQ 分别是 UART（串行接口）和 VirtIO 磁盘（虚拟存储设备）的中断源。
  // 优先级条目占用 32 位（4 字节）
  *(uint32 *)(PLIC + UART0_IRQ * 4) = 1;
  *(uint32 *)(PLIC + VIRTIO0_IRQ * 4) = 1;
}

void plic_inithart(void)
{
  int hart = cpuid();
  // set enable bits for this hart's S-mode
  // for the uart and virtio disk.
  *(uint32 *)PLIC_SENABLE(hart) = (1 << UART0_IRQ) | (1 << VIRTIO0_IRQ);


  // set this hart's S-mode priority threshold to 0.
  *(uint32 *)PLIC_SPRIORITY(hart) = 0;
}

// ask the PLIC what interrupt we should serve.
int plic_claim(void)
{
  int hart = cpuid();
  int irq = *(uint32*)PLIC_SCLAIM(hart);
  return irq;
}

// tell the PLIC we've served this IRQ.
void plic_complete(int irq)
{
  int hart = cpuid();
  *(uint32*)PLIC_SCLAIM(hart) = irq;
}