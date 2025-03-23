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

inline uint32 calculate_order(uint32 size)
{
    if (size == 0)
        return 0; // 特殊情况，大小为0时返回0

    uint32 order = 0;

    // 将 size 向上舍入到最接近的 2 的幂
    size--; // 先减 1，确保当 size 是 2 的幂时能正确地映射到对应的 order
    while (size > 0)
    {
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
