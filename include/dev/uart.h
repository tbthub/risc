#ifndef __CHAR_UART_H__
#define __CHAR_UART_H__
void uart_putc_sync(int c);
void uart_init();
void uartintr();
#endif