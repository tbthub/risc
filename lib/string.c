#include "std/stddef.h"

/*
 *
 * 该文件包含一些对内存的操作，比如
 * memset、memcmp、memmove、memcpy
 * 以及一些字符串操作
 * strncmp、strncpy、safestrcpy、strlen
 * 暂时没有完全实现，用到再说
 *
 */

// 在 dst 处设置 n 个 c
void *memset(void *dst, int c, uint64 n)
{
    char *cdst = (char *)dst;
    int i;
    for (i = 0; i < n; i++)
        cdst[i] = c;
    return dst;
}

void *memcpy(void *dst, const void *src, uint64 n)
{
    unsigned char *_dst = (unsigned char *)dst;
    const unsigned char *_src = (const unsigned char *)src;

    for (uint64 i = 0; i < n; i++)
        _dst[i] = _src[i];

    return _dst;  // 返回目标指针
}

int strdup(void *dst, const void *src)
{
    char *_dst = (char *)dst;
    char *_src = (char *)src;
    int len = 0;
    while ((*_dst++ = *(_src + len++)))
        ;
    return len;
}

int strncpy(void *dst, const void *src, uint16 len)
{
    char *_dst = (char *)dst;
    char *_src = (char *)src;
    int copied = 0;

    while (copied < len && (*_dst++ = *_src++)) {
        copied++;
    }

    if (copied < len)
        *_dst = '\0';
    return copied;  // 返回复制的字符数
}

int strncmp(const char *p, const char *q, uint n)
{
    while (n > 0 && *p && *p == *q)
        n--, p++, q++;
    if (n == 0)
        return 0;
    return (uchar)*p - (uchar)*q;
}

int strcmp(const char *p, const char *q)
{
    while (*p && *p == *q) {
        p++;
        q++;
    }
    return *(uchar *)p - *(uchar *)q;
}

// 字符串哈希函数，使用 DJB2 算法
int strhash(const char *str)
{
    unsigned long hash = 5381;

    while (*str) {
        hash = ((hash << 5) + hash) + (unsigned char)(*str);  // hash * 33 + c
        str++;
    }

    return (int)(hash & 0x7FFFFFFF);  // 保证哈希值是正数
}

uint32 strlen(const char *p)
{
    const char *start = p;  // 记录起始位置
    while (*p)
        p++;
    return (p - start);
}
