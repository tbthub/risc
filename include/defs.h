#ifndef __DEFS_H__
#define __DEFS_H__

#include "std/stddef.h"
#include "mm/mm.h"
#include "mm/page.h"

extern char etext[];      // kernel.ld sets this to end of kernel code.
extern char trampoline[]; // trampoline.S
extern char end[];




#endif