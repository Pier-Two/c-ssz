#include "../include/ssz_utils.h"
#include "../include/ssz_constants.h"

/**
 * Returns true if the given value is a power of two, and false otherwise.
 * A neat trick is to use the fact that (x & (x-1)) == 0 if and only if x is a power of two
 * (and x is not zero). We still treat zero as not a power of two in this function,
 * unless you want to handle that differently for some reason.
 */
bool is_power_of_two(uint64_t value)
{
    if (value == 0)
    {
        return false;
    }
    return (value & (value - 1)) == 0;
}

/**
 * Returns the next power of two for the given value. If the value is already a power of two,
 * it returns the same value. If the value is zero, we define it to return one.
 * Otherwise, we repeatedly shift until we've passed the highest set bit in 'value'.
 */
uint64_t next_pow_of_two(uint64_t value)
{
    if (value == 0)
    {
        return 1;
    }
    // If value is already a power of two, just return it
    if (is_power_of_two(value))
    {
        return value;
    }
    // Repeatedly shift right until the value is zero, counting how many shifts that takes,
    // then shift left that many times to get the next power of two.
    uint64_t p = 1;
    while (p < value)
    {
        p <<= 1;
    }
    return p;
}
