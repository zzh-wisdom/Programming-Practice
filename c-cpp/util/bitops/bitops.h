#pragma once

#include <stdint.h>
/*
 *  Determine whether some value is a power of two, where zero is
 * *not* considered a power of two.
 */

static inline __attribute__((const))
bool is_power_of_2(uint32_t n)
{
	return (n != 0 && ((n & (n - 1)) == 0));
}

static inline uint32_t next_power_of_2(uint32_t num){
    uint32_t max = 1 << 31;
    uint32_t n = num - 1;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return num <= 0 ? 1 : (n >= max ? max : n + 1);
}
