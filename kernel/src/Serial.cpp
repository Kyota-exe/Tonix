#include "Serial.h"
#include "IO.h"
#include "Spinlock.h"

constexpr uint16_t PORT = 0xe9;

Spinlock lock;
void Serial::Print(const char* string, const char* end)
{
    lock.Acquire();
    while (*string != 0) outb(PORT, *string++);
    while (*end != 0) outb(PORT, *end++);
    lock.Release();
}

void Serial::Print(const String& string, const char* end)
{
    Print(string.ToCString(), end);
}