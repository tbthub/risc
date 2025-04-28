#ifndef __STRING_H__
#define __STRING_H__
#include "std/stddef.h"

extern void *memset(void *dst, int c, uint64 n);
extern void *memcpy(void *dst, const void *src, uint64 n);

extern int strdup(void *dst, const void *src);
extern int strcmp(const char *p, const char *q);
extern int strncpy(void *dst, const void *src,uint16 len);
extern int strncmp(const char *p, const char *q, uint n);
extern uint32 strlen(const char *p);


extern int strhash(const char *str);


#endif
