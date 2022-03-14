#include "StringUtilities.h"

namespace StringUtils
{
    uint64_t ToString(char* buffer, int64_t n)
    {
        if (n == 0)
        {
            buffer[0] = '0';
            return 1;
        }

        uint8_t size = 0;
        int64_t num = n;
        while (num)
        {
            size++;
            num /= 10;
        }

        for (uint8_t i = size; i > 0; --i)
        {
            buffer[i - 1] = (char)(n % 10 + '0');
            n /= 10;
        }

        return size;
    }

    uint64_t ToHexString(char* buffer, uint64_t n)
    {
        if (n == 0)
        {
            buffer[0] = '0';
            return 1;
        }

        uint8_t size = 0;
        uint64_t num = n;
        while (num)
        {
            size++;
            num /= 16;
        }

        for (uint8_t i = size; i > 0; --i)
        {
            buffer[i - 1] = "0123456789abcdef"[n % 16];
            n /= 16;
        }

        return size;
    }

    void Format(char* buffer, const char* string, int64_t value)
    {
        const char* c = string;
        char* bufferPtr = buffer;

        bool nextIsFormatCode = false;
        while (*c != 0)
        {
            if (nextIsFormatCode)
            {
                switch (*c++)
                {
                    case 'd':
                    {
                        bufferPtr += ToString(bufferPtr, value);
                        nextIsFormatCode = false;
                        continue;
                    }
                    case 'x':
                    {
                        bufferPtr += ToHexString(bufferPtr, value);
                        nextIsFormatCode = false;
                        continue;
                    }
                    default:
                    {
                        nextIsFormatCode = false;
                        continue;
                    }
                }
            }

            if (*c == '%')
            {
                nextIsFormatCode = true;
                c++;
                continue;
            }

            *bufferPtr++ = *c++;
        }
        *bufferPtr = 0;
    }
}