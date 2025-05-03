#include "lib/math.h"
#include "std/stddef.h"
inline uint64 next_power_of_2(uint64 n)
{
    if (n == 0)
        return 1; // 特殊情况
    n--;          // 先减1，确保处理非2的幂时能扩展到正确的2的幂
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32; // 针对64位整数的最高位
    return n + 1; // 加1得到下一个2的幂
}

inline uint32 math_order2(uint32 size)
{
    if (size == 0) return 0;

    // 1. 向上取整到最近的 2 的幂
    size--;
    size |= size >> 1;
    size |= size >> 2;
    size |= size >> 4;
    size |= size >> 8;
    size |= size >> 16;
    size++;

    // 2. 计算阶数
    uint32 order = 0;
    while (size > 1) {
        size >>= 1;
        order++;
    }
    return order;
}

inline uint32 math_log(uint32 num, uint16 base)
{
    uint32 result = 0;

    // 如果底数为1或者num小于1则没有有效结果
    if (base <= 1 || num <= 0)
        return 0;

    // 计算对数，直到num小于base
    while (num >= base)
    {
        num /= base;
        result++;
    }

    return result;
}

inline uint32 math_pow(uint32 base, uint16 exponent)
{
    uint32 result = 1;

    // 如果指数是0，任何数的0次方都为1
    if (exponent == 0)
        return 1;

    // 如果指数是负数，这里不处理，假设 exponent >= 0
    while (exponent > 0)
    {
        result *= base;
        exponent--;
    }

    return result;
}
