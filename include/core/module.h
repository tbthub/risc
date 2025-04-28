#ifndef __MODULE_H__
#define __MODULE_H__

#define INIT_SECTION ".mod_init"
#define EXIT_SECTION ".mod_exit"

extern char kmod_init_start[];
extern char kmod_init_end[];

typedef int (*initcall_t)(void);
typedef void (*exitcall_t)(void);

#define IN_MOD 1
#define RM_MOD 0

// #define __init  __attribute__((__used__, section(INIT_SECTION)))
// #define __exit  __attribute__((__used__, section(EXIT_SECTION)))

#define module_init(initfn) static initcall_t __initcall_##initfn __attribute__((__used__, section(INIT_SECTION))) = initfn
#define module_exit(exitfn) static exitcall_t __exitcall_##exitfn __attribute__((__used__, section(EXIT_SECTION))) = exitfn

#endif
