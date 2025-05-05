#include "core/locks/spinlock.h"
#include "dev/uart.h"
#include "dev/cons.h"

// TODO 这个比较特殊，暂时还不具备通用性。


struct cons_struct cons;

void cons_init()
{
    spin_init(&cons.lock, "console");
    uart_init();
}
