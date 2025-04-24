#include "conf.h"
#include "core/syscall.h"

# 用户程序的入口、出口代码

.globl _start
_start:
    li ra, USER_TEXT_BASE
    jalr ra
    
    # Exit (unreachable in this case)
    li a0, 0
    li a7, SYS_exit
    ecall
