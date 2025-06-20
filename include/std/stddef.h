#ifndef __STDDEF_H
#define __STDDEF_H
#include "conf.h"

#define NULL ((void *)0)

#define ERR -1

// 成员 member 在 type 中的偏移量
#define offsetof(TYPE, MEMBER) ((uint64_t) & ((TYPE *)0)->MEMBER)

// 包含该成员 member 的结构体 type 的起始位置指针，经过ptr地址计算
#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;

typedef char int8;
typedef short int16;
typedef int int32;
typedef long int64;

typedef uint64_t pde_t;
typedef uint32_t dev_t;
typedef uint32_t mode_t;
typedef uint32_t flags_t;
typedef uint64_t flags64_t;
typedef int fd_t;



// typedef unsigned char uint8_t;
// typedef unsigned short uint16_t;
// typedef unsigned int uint32_t_t;
// typedef unsigned long uint64_t_t;

// typedef char int8_t;
// typedef short int16_t;
// typedef int int32_t;
// typedef long int64_t;

#define VM_PROT_NONE (1L << 0)
#define VM_PROT_READ (1L << 1)
#define VM_PROT_WRITE (1L << 2)
#define VM_PROT_EXEC (1L << 3)
#define VM_PROT_LAZY (1L << 31)

#define SET_FLAG(flags, flag) (*(flags) |= (flag))
#define CLEAR_FLAG(flags, flag) (*(flags) &= ~(flag))
#define TEST_FLAG(flags, flag) (*(flags) & (flag))

#define min(a, b) ((a) < (b) ? (a) : (b))

#define ENOSYS 38

typedef int pid_t;
typedef int tid_t;

#endif
