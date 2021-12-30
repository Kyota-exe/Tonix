#include "Serial.h"

void Serial::Print(const char* string, const char* end)
{
    outb(PORT, (uint8_t*)string, StringLength(string));
    outb(PORT, (uint8_t*)end, StringLength(end));
}

void Serial::Printf(const char* string, int64_t value, const char* end)
{
    Print(FormatString(string, value), end);
}