#include "core/proc.h"
#include "defs.h"
#include "dev/cons.h"
#include "dev/uart.h"
#include "core/locks/spinlock.h"
#include "riscv.h"
#include "std/stdarg.h"
#include "std/stdio.h"

// 该文件临时用来打印一些调试信息

void putchar(char c)
{
    uart_putc_sync(c);
}

void putint(long n)
{
    if (n < 0) {  // 处理负数
        putchar('-');
        n = -n;
    }
    if (n / 10) {  // 如果不是个位数，递归处理高位
        putint(n / 10);
    }
    putchar('0' + n % 10);  // 打印当前位
}

void putstr(char *str)
{
    while (*str) {
        uart_putc_sync(*str++);
    }
}

void putptr(void *ptr)
{
    uint64 uptr = (uint64)ptr;
    putstr("0x");  // 输出十六进制前缀

    // 按十六进制逐位输出，从高位到低位
    for (int i = (sizeof(uptr) * 2) - 1; i >= 0; i--) {
        uint8 digit = (uptr >> (i * 4)) & 0xF;  // 提取当前位
        if (digit < 10)
            putchar('0' + digit);  // 数字0-9
        else
            putchar('a' + (digit - 10));  // 字母a-f
    }
}

// 暂时用于调试，后期会改
void printf(const char *fmt, va_list ap)
{
    char c0, c1;
    for (int i = 0; fmt[i]; i++) {
        c0 = fmt[i];
        if (c0 == '%') {
            if (fmt[i + 1] == '\0') {
                break;
            }
            c1 = fmt[++i];
            switch (c1) {
            case 'd':
                putint(va_arg(ap, int));
                break;
            case 'c':
                putchar(va_arg(ap, int));
                break;
            case 's':
                putstr(va_arg(ap, char *));
                break;
            case 'p':
                putptr(va_arg(ap, void *));
                break;
            default:
                i--;
            }
        }
        else {
            putchar(c0);
        }
    }
}

void printk(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    spin_lock(&cons.lock);
    printf(fmt, ap);
    spin_unlock(&cons.lock);
    va_end(ap);
}

// 关中断，死循环
void panic(const char *fmt, ...)
{
    intr_off();
    struct thread_info *t = myproc();
    printk("[panic]: !!!\n", "[panic]: panic on hart: %d\n", cpuid());
    if (t)
        printk("[panic]: pid: %d, name:%s\n", t->pid, t->name);

    va_list ap;
    va_start(ap, fmt);
    spin_lock(&cons.lock);
    printf(fmt, ap);
    spin_unlock(&cons.lock);
    va_end(ap);
    putstr("[panic]: !!!\n");
    for (;;)
        ;
}

void assert(int condition, const char *fmt, ...)
{
    if (!condition) {
        intr_off();
        struct thread_info *t = myproc();
        printk("[panic]: !!!\n", "[panic]: panic on hart: %d\n", cpuid());
        if (t)
            printk("[panic]: pid: %d, name:%s\n", t->pid, t->name);

        va_list ap;
        va_start(ap, fmt);
        spin_lock(&cons.lock);
        printf(fmt, ap);
        spin_unlock(&cons.lock);
        va_end(ap);
        putstr("[panic]: !!!\n");
        for (;;)
            ;
    }
}
