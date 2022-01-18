#include "StringUtilities.h"

namespace String
{
    static char bufferToString[32];
    const char* ToString(int64_t n)
    {
        if (n == 0) return "0";

        bool isNegative = n < 0;
        if (isNegative)
        {
            n = -n;
            bufferToString[0] = '-';
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
            bufferToString[i - 1] = (char)(n % 10 + '0');
            n /= 10;
        }
        bufferToString[size + isNegative] = 0;

        return bufferToString;
    }

    const char* ToHexString(uint64_t n)
    {
        if (n == 0) return "0";

        bufferToString[0] = '0';
        bufferToString[1] = 'x';

        uint8_t size = 0;
        uint64_t num = n;
        while (num)
        {
            size++;
            num /= 16;
        }

        for (uint8_t i = size + 2; i > 2; --i)
        {
            bufferToString[i - 1] = "0123456789abcdef"[n % 16];
            n /= 16;
        }
        bufferToString[size + 2] = 0;

        return bufferToString;
    }

    uint64_t Length(const char* string, uint64_t max)
    {
        uint64_t length = 0;
        while (*string++ != 0 && (length < max || max == 0))
        {
            length++;
        }
        return length;
    }

    char bufferFormatString[128];
    const char* Format(const char* string, int64_t value)
    {
        const char* c = string;
        char* bufferPtr = bufferFormatString;

        bool nextIsFormatCode = false;
        while (*c != 0)
        {
            if (nextIsFormatCode)
            {
                switch (*c++)
                {
                    case 'd':
                    {
                        const char* intString = ToString(value);
                        while (*intString != 0)
                        {
                            *bufferPtr++ = *intString++;
                        }
                        nextIsFormatCode = false;
                        continue;
                    }
                    case 'x':
                    {
                        const char* hexString = ToHexString(value);
                        while (*hexString != 0)
                        {
                            *bufferPtr++ = *hexString++;
                        }
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

        return bufferFormatString;
    }

    // Behaves like strtok_r
    char* Split(char* string, char splitCharacter, char** stringPtr)
    {
        char *end = string;
        while (*end != splitCharacter && *end != 0) end++;
        if (*end == 0)
        {
            *stringPtr = end;
            return string;
        }
        *end = 0;
        *stringPtr = end + 1;
        return string;
    }

    bool Equals(const char* s1, const char* s2)
    {
        const char* ptr1 = s1;
        const char* ptr2 = s2;

        while (*ptr1 != 0 && *ptr2 != 0)
        {
            if (*ptr1++ != *ptr2++) return false;
        }

        return *ptr1 == 0 && *ptr2 == 0;
    }

    uint64_t Count(const char* string, char character)
    {
        uint64_t result = 0;
        while (*string != 0)
        {
            if (*string++ == character) result++;
        }
        return result;
    }

    uint64_t ToUInt(const char* string)
    {
        uint64_t number = 0;
        while (*string != 0)
        {
            number *= 10;
            number += *string - '0';
            string++;
        }
        return number;
    }

    uint64_t OctalToUInt(const char* string)
    {
        uint64_t number = 0;
        while (*string != 0)
        {
            number *= 8;
            number += *string - '0';
            string++;
        }
        return number;
    }
}