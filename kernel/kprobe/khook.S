    .globl  kprobe_exec_entry
    .global kprobe_exec_entry_set
    .global kprobe_attch_entry
    .global kprobe_attch_entry_set

    # 下面两个仅仅作为符号确定位置，这种东西我们懒得专门分配一个节
    # 当我们有多个探针的时候，我们申请一个页面，每个页面的代码都是这个汇编
    # （需要修改 _entry_set入口，目前仅支持32位跳转，+- 2G）
    .global kprobe_start
    .global kprobe_end

    .align  4


kprobe_start:
# 这里是用
#  auipc t0, imm1
# jalr t0,imm2(t0)
# 跳转过来的，返回值保存在 t0 寄存器中
# 我们后面需要通过 t0 返回
# 根据ABI约定，作为调用者，我们需要保存 t0 后才能跳转
kprobe_exec_entry:
# save caller-saved registers.
    addi    sp, sp, -144
    sd      ra, 0(sp)
    sd      gp, 8(sp)
    sd      s0, 16(sp)
    sd      t0, 24(sp)
    sd      t1, 32(sp)
    sd      t2, 40(sp)
    sd      a0, 48(sp)
    sd      a1, 56(sp)
    sd      a2, 64(sp)
    sd      a3, 72(sp)
    sd      a4, 80(sp)
    sd      a5, 88(sp)
    sd      a6, 96(sp)
    sd      a7, 104(sp)
    sd      t3, 112(sp)
    sd      t4, 120(sp)
    sd      t5, 128(sp)
    sd      t6, 136(sp)

# auipc t0, imm20 # t0 ← PC + imm20 << 12
# jalr ra, imm12(t0) # 跳转到 t0 + imm12，ra 保存返回地址
kprobe_exec_entry_set:
    auipc   t0,0
    jalr    ra,0(t0)

# restore registers.
    ld      ra, 0(sp)
    ld      gp, 8(sp)
    ld      s0, 16(sp)
    ld      t0, 24(sp)
    ld      t1, 32(sp)
    ld      t2, 40(sp)
    ld      a0, 48(sp)
    ld      a1, 56(sp)
    ld      a2, 64(sp)
    ld      a3, 72(sp)
    ld      a4, 80(sp)
    ld      a5, 88(sp)
    ld      a6, 96(sp)
    ld      a7, 104(sp)
    ld      t3, 112(sp)
    ld      t4, 120(sp)
    ld      t5, 128(sp)
    ld      t6, 136(sp)
    addi    sp, sp, 144

    # 根据 t0 的值跳转回去
    jalr    zero, t0, 0




# 这里跳转方式类上
# 不过我们后面不需要保存 t0 的值
# 原函数的返回值保存在 ra（注意我们跳转方式并没有修改成新的 ra）
# 我们直接返回，这样原函数就不会被执行了
kprobe_attch_entry:
    addi    sp, sp, -8
    sd      ra, 0(sp)

# auipc t0, imm20 # t0 ← PC + imm20 << 12
# jalr ra, imm12(t0) # 跳转到 t0 + imm12，ra 保存返回地址
kprobe_attch_entry_set:
    auipc   t0,0
    jalr    ra,0(t0)

    ld      ra, 0(sp)
    addi    sp, sp,8
    ret

kprobe_end:
