#include "StringUtilities.h"

static char buffer[32];

char* ToString(int64_t n)
{
    if (n == 0)
    {
        buffer[0] = '0';
        buffer[1] = 0;
        return buffer;
    }

    bool isNegative = n < 0;
    if (isNegative)
    {
        n = -n;
        buffer[0] = '-';
    }

    uint8_t size = 0;
    int64_t num = n;
    while (num)
    {
        size++;
        num /= 10;
    }

    for (uint8_t i = size + isNegative; i > isNegative; --i)
    {
        buffer[i - 1] = n % 10 + '0';
        n /= 10;
    }
    buffer[size + isNegative] = 0;

    return buffer;
}

char* ToHexString(uint64_t n)
{
    if (n == 0)
    {
        buffer[0] = '0';
        buffer[1] = 0;
        return buffer;
    }

    buffer[0] = '0';
    buffer[1] = 'x';

    uint8_t size = 0;
    int64_t num = n;
    while (num)
    {
        size++;
        num /= 16;
    }

    for (uint8_t i = size + 2; i > 2; --i)
    {
        buffer[i - 1] = "0123456789ABCDEF"[n % 16];
        n /= 16;
    }
    buffer[size + 2] = 0;

    return buffer;
}

size_t StringLength(const char* string)
{
    size_t length = 0;
    while (*string++ != '\0')
    {
        length++;
    }
    return length;
}