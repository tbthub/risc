#ifndef _KERNEL_EXPORT_H
#define _KERNEL_EXPORT_H


struct kernel_symbol {
    const char *name; 
    void *addr; 
} __attribute__((packed));

/* 导出符号宏 */
#define EXPORT_SYMBOL(sym)                              \
    extern typeof(sym) sym;                             \
    __attribute__((used, section("__ksymtab")))         \
    static const struct kernel_symbol __ksymtab_##sym = \
    { .name = #sym, .addr = (void*)&sym }

/* 提供符号查找函数 */
const struct kernel_symbol *lookup_symbol(const char *name);

#endif
