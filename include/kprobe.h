#ifndef __KPROBE_H__
#define __KPROBE_H__
#include "std/stddef.h"

#define auipc_jalr_len 8

struct kprobe
{
    void *page;
    char *src[auipc_jalr_len];
    int type;
    uint16 *entry;
};

extern struct kprobe *kprobe_exec(void *entry_func, void *my_func);
extern struct kprobe *kprobe_attch(void *entry_func, void *my_func);

// * 内核模块使用下面这两个
// * 前提是 entry_func 被导出
// ! 强烈**不建议**模块内部使用 extern 函数，这样会造成污染
extern struct kprobe *mod_kprobe_exec(const char *symname, void *my_func);
extern struct kprobe *mod_kprobe_attch(const char *symname, void *my_func);

extern void kprobe_clear(struct kprobe *kp);

#endif
