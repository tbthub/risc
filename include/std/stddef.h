#ifndef __STDDEF_H
#define __STDDEF_H
#include "conf.h"

#define NULL ((void *)0)

#define ERR -1

// 成员 member 在 type 中的偏移量
#define offsetof(TYPE, MEMBER) ((uint64) & ((TYPE *)0)->MEMBER)

// 包含该成员 member 的结构体 type 的起始位置指针，经过ptr地址计算
#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long uint64;

typedef char int8;
typedef short int16;
typedef int int32;
typedef long int64;

typedef uint64 pde_t;
typedef uint32 dev_t;
typedef uint32 mode_t;
typedef uint32 flags_t;
typedef uint64 flags64_t;
typedef int fd_t;

typedef int64 ssize_t;
typedef uint64 size_t;

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
