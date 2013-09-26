#include <canopus/math_utils.h>
#include <stdint.h>

/* i ^ n */
int
ipow(uint32_t n, uint32_t i)
{
    uint32_t value = i;
    if (!n) return 1;
    while (--n) value *= i;
    return value;
}
