#include "core/syscall.h"

    .section user_entry_point
    .align   4
    .globl   user_entry
    .globl   sigact_call

# 用户程序的实际入口，调用用户的 ph.entry后返回，然后exit
# 这样用户代码就不用显式 exit
user_entry:
    jalr     a0

# Exit
    li       a0, 0
    li       a7, SYS_exit
    ecall

# 用于跳转到用户自定义信号处理函数，以及信号返回
# 就这点代码，索性直接放 start.S 这里了
sigact_call:
    jalr a0
    li  a7, SYS_sigret
    ecall
    
