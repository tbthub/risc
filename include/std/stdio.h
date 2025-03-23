#ifndef __STDIO_H__
#define __STDIO_H__
#include "std/stdarg.h"
#include "lib/spinlock.h"
#include "std/stddef.h"
#include "dev/cons.h"
#define INPUT_BUF_SIZE 128

// void putchar(char c);
// void putint(long n);
// void putstr(char *str);
// void putptr(void *ptr);
extern void printk(const char *fmt, ...);
extern void panic(const char *fmt, ...);
extern void assert(int condition, const char *fmt, ...);
#endif