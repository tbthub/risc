#ifndef __STDARG_H__
#define __STDARG_H__

#define va_start(v, l) __builtin_va_start(v, l)
#define va_end(v) __builtin_va_end(v)
#define va_arg(v, l) __builtin_va_arg(v, l)

typedef __builtin_va_list __gnuc_va_list;
typedef __gnuc_va_list va_list;

struct args
{
    int args_count;
    void *arg1;
    void *arg2;
    void *arg3;
    void *arg4;
    void *arg5;
    void *arg6;
};

#endif