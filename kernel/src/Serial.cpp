#include "Serial.h"
#include "IO.h"

constexpr uint16_t PORT = 0xe9;

void Serial::Print(const char* string, const char* end)
{
    while (*string != 0) outb(PORT, *string++);
    while (*end != 0) outb(PORT, *end++);
}

void Serial::Print(const String& string, const char* end)
{
    Print(string.ToCString(), end);
}