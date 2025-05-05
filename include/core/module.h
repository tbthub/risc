#ifndef __MODULE_H__
#define __MODULE_H__

extern char kmod_init_start[];
extern char kmod_init_end[];

typedef int (*initcall_t)(void);
typedef void (*exitcall_t)(void);

#define IN_MOD 1
#define RM_MOD 0

// 1. 定义段名
#define KINIT_SECTION ".kmod_init"
#define KEXIT_SECTION ".kmod_exit"

// 2. 定义初始化级别
#define INIT_LEVEL_EARLY   0    // 早期初始化（如内存管理）
#define INIT_LEVEL_CORE    1    // 核心子系统（如文件系统）
#define INIT_LEVEL_DEVICE  2    // 设备驱动
#define INIT_LEVEL_LATE    3    // 后期初始化
#define INIT_LEVEL_FS      4    // 文件系统挂载
#define INIT_LEVEL_LAST    5    // 最后阶段

#define INIT_LEVEL_NUM     6
#define INIT_LEVEL_DEFAULT INIT_LEVEL_DEVICE
#define EXIT_LEVEL_DEFAULT INIT_LEVEL_DEVICE

// 3. 字符串工具宏
#define STR(x) #x
#define __concat(a, b) a b

#define module_init_level(initfn, level)  \
    static initcall_t __initcall_##initfn \
    __attribute__((__used__, section(__concat(KINIT_SECTION, STR(level))))) = initfn

#define module_exit_level(exitfn, level)  \
    static exitcall_t __exitcall_##exitfn \
    __attribute__((__used__, section(__concat(KEXIT_SECTION, STR(level))))) = exitfn


#define module_init(initfn) module_init_level(initfn,INIT_LEVEL_DEVICE)
#define module_exit(exitfn) module_exit_level(exitfn,EXIT_LEVEL_DEFAULT)

extern const struct kernel_symbol *lookup_symbol(const char *name);


#endif
