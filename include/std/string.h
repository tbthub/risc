#ifndef __STRING_H__
#define __STRING_H__
#include "std/stddef.h"
#include "std/stdint.h"

extern void *memset(void *dst, int c, uint64_t n);
extern void *memcpy(void *dst, const void *src, uint64_t n);

extern int strdup(void *dst, const void *src);
extern int strncpy(void *dst, const void *src,uint16_t len);
extern int strncmp(const char *p, const char *q, uint32_t n);
extern uint32_t strlen(const char *p);
extern int strhash(const char *str);
char *strcpy(char *dest, const char *src);
int strcmp(const char *a, const char *b);
int memcmp(const void *a, const void *b, size_t size);
char *strncat(char *__restrict dest, const char *src, size_t max_size);
char *strchr(const char *s, int c);
char *strcat(char *dest, const char *src);
#endif
