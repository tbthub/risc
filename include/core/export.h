#ifndef _KERNEL_EXPORT_H
#define _KERNEL_EXPORT_H
#include "lib/hash.h"

struct kernel_symbol
{
    const char *name;
    void *addr;
} __attribute__((packed));

struct ksym
{
    struct kernel_symbol *ksp;
    hash_node_t node;
};

/* 导出符号宏 */
#define EXPORT_SYMBOL(sym)                                                                                                                                                                             \
    extern typeof(sym) sym;                                                                                                                                                                            \
    __attribute__((used, section("__ksymtab"))) static const struct kernel_symbol __ksymtab_##sym = {.name = #sym, .addr = (void *)&sym}

extern struct kernel_symbol __start___ksymtab[];
extern struct kernel_symbol __stop___ksymtab[];

#endif
