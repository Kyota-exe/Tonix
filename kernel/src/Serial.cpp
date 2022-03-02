#include "Serial.h"
#include "IO.h"
#include "StringUtilities.h"

constexpr uint16_t PORT = 0xe9;

void Serial::Print(const char* string, const char* end)
{
    outb(PORT, (uint8_t*)string, StringUtils::Length(string));
    outb(PORT, (uint8_t*)end, StringUtils::Length(end));
}

void Serial::Print(const String& string, const char* end)
{
    Print(string.ToCString(), end);
}