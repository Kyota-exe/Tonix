#include "Serial.h"
#include "IO.h"
#include "Spinlock.h"

constexpr uint16_t PORT = 0xe9;

uint64_t ToRawString(char* buffer, uint64_t number, unsigned int base)
{
    Assert(base > 0);

    if (number == 0)
    {
        buffer[0] = '0';
        return 1;
    }

    Assert(base <= 16);

    uint64_t size = 0;
    uint64_t num = number;
    while (num > 0)
    {
        size++;
        num /= base;
    }

    num = number;
    for (uint64_t i = size; i-- > 0; )
    {
        buffer[i] = "0123456789abcdef"[num % base];
        num /= base;
    }

    return size;
}

void Serial::Printf(const char* string, ...)
{
    va_list args;
    va_start(args, string);

    char formatBuffer[128];
    char* bufferPtr = formatBuffer;

    const char* c = string;
    bool nextIsFormatCode = false;
    while (*c != 0)
    {
        if (nextIsFormatCode)
        {
            switch (*c)
            {
                case 'd':
                {
                    bufferPtr += ToRawString(bufferPtr, va_arg(args, uint64_t), 10);
                    break;
                }
                case 'x':
                {
                    bufferPtr += ToRawString(bufferPtr, va_arg(args, uint64_t), 16);
                    break;
                }
                case 's':
                {
                    const char* s = va_arg(args, const char*);
                    while (*s != 0) *bufferPtr++ = *s++;
                    break;
                }
            }
            nextIsFormatCode = false;
        }
        else if (*c == '%')
        {
            Assert(!nextIsFormatCode);
            nextIsFormatCode = true;
        }
        else
        {
            *bufferPtr++ = *c;
        }
        c++;
    }
    *bufferPtr = 0;

    va_end(args);

    static Spinlock lock;
    lock.Acquire();
    bufferPtr = formatBuffer;
    while (*bufferPtr != 0) outb(PORT, *bufferPtr++);
    outb(PORT, '\n');
    lock.Release();
}