#include "Math.h"

uint64_t Pow(uint64_t base, uint64_t exponent)
{
    uint64_t result = 1;
    for (uint64_t i = exponent; i-- > 0; )
    {
        result *= base;
    }

    return result;
}

uint64_t CeilLog2(uint64_t value)
{
    // Don't ask.

    static const uint64_t t[6]
    {
        0xFFFFFFFF00000000,
        0x00000000FFFF0000,
        0x000000000000FF00,
        0x00000000000000F0,
        0x000000000000000C,
        0x0000000000000002
    };

    uint64_t output = (((value & (value - 1)) == 0) ? 0 : 1);

    uint8_t j = 32;
    for (uint64_t i : t)
    {
        int k = (((value & i) == 0) ? 0 : j);
        output += k;
        value >>= k;
        j >>= 1;
    }

    return output;
}