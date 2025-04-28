#include "core/export.h"
#include "std/stddef.h"
#include "lib/string.h"

/* 声明符号表的起始和结束地址（由链接脚本定义） */
extern struct kernel_symbol __start___ksymtab[];
extern struct kernel_symbol __stop___ksymtab[];

/* 根据符号名查找地址 */
const struct kernel_symbol *lookup_symbol(const char *name) {
    for (struct kernel_symbol *sym = __start___ksymtab; 
         sym < __stop___ksymtab; 
         sym++) {
        if (strcmp(sym->name, name) == 0) {
            return sym;
        }
    }
    return NULL;
}
