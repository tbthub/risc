#
# low-level code to handle traps from user space into
# the kernel, and returns from kernel to user.
#
# the kernel maps the page holding this code
# at the same virtual address (TRAMPOLINE)
# in user and kernel space so that it continues
# to work when it switches page tables.
# kernel.ld causes this code to start at
# a page boundary.
#
# * 参考 info.md/用户中断

#include "riscv.h"
#include "mm/memlayout.h"

        .section uservec_space
        .align   4
        .globl   usertrap
        .globl   uservec
uservec:

# * sscratch 存放的是当前进程的 trapframe 地址
# * 当前的寄存器现场保存到 trapframe
# * 交换 a0, sscratch 的值，此时 a0 为 p->tf
        csrrw    a0, sscratch, a0

# * 把寄存器保存到内核栈中
        sd       ra, 16(a0)
        sd       sp, 24(a0)       # * 此时 sp 是用户栈顶
        sd       gp, 32(a0)
        # sd       tp, 40(a0)
        sd       t0, 48(a0)
        sd       t1, 56(a0)
        sd       t2, 64(a0)
        sd       s0, 72(a0)
        sd       s1, 80(a0)
# sd a0, 88(a0) 占位 88
        sd       a1, 96(a0)
        sd       a2, 104(a0)
        sd       a3, 112(a0)
        sd       a4, 120(a0)
        sd       a5, 128(a0)
        sd       a6, 136(a0)
        sd       a7, 144(a0)
        sd       s2, 152(a0)
        sd       s3, 160(a0)
        sd       s4, 168(a0)
        sd       s5, 176(a0)
        sd       s6, 184(a0)
        sd       s7, 192(a0)
        sd       s8, 200(a0)
        sd       s9, 208(a0)
        sd       s10, 216(a0)
        sd       s11, 224(a0)
        sd       t3, 232(a0)
        sd       t4, 240(a0)
        sd       t5, 248(a0)
        sd       t6, 256(a0)

# 初始化内核栈栈指针
        ld       sp,0 (a0)

# 先保存原来 a0 的值到栈中
        csrr     t0, sscratch     # 读取原来的 a0 (即 sscratch 的值) 到 t0
        sd       t0, 88(a0)       # 保存原来的 a0 到栈中

        csrw     sscratch, a0     # 写回 p->tf 到 sscratch

# 用户程序不需要返回，采用无条件跳转
        j        usertrap




        .globl   userret
userret:
# userret(pagetable)
# called by usertrapret() in trap.c to
# switch from kernel to user.


# * 读取 sscratch 的值，此时 a0 为 p->tf
        csrr     a0, sscratch

# * 把寄存器加载
        ld       ra, 16(a0)
        ld       sp, 24(a0)
        ld       gp, 32(a0)
        # ld       tp, 40(a0)
        ld       t0, 48(a0)
        ld       t1, 56(a0)
        ld       t2, 64(a0)
        ld       s0, 72(a0)
        ld       s1, 80(a0)
# ld a0, 88(a0) 占位 88
        ld       a1, 96(a0)
        ld       a2, 104(a0)
        ld       a3, 112(a0)
        ld       a4, 120(a0)
        ld       a5, 128(a0)
        ld       a6, 136(a0)
        ld       a7, 144(a0)
        ld       s2, 152(a0)
        ld       s3, 160(a0)
        ld       s4, 168(a0)
        ld       s5, 176(a0)
        ld       s6, 184(a0)
        ld       s7, 192(a0)
        ld       s8, 200(a0)
        ld       s9, 208(a0)
        ld       s10, 216(a0)
        ld       s11, 224(a0)
        ld       t3, 232(a0)
        ld       t4, 240(a0)
        ld       t5, 248(a0)
        ld       t6, 256(a0)
        ld       a0, 88(a0)

# return to user mode and user pc.
# usertrapret() set up sstatus and sepc.
        sret
