#include "lib/spinlock.h"
#include "dev/uart.h"
#include "dev/cons.h"

struct cons_struct cons;

void cons_init()
{
    spin_init(&cons.lock, "console");
    uart_init();
}
