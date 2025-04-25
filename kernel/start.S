#include "core/syscall.h"

    .section user_entry_point
    .align   4
    .globl   user_entry
user_entry:
    jalr     a0 

# Exit
    li       a0, 0
    li       a7, SYS_exit
    ecall
