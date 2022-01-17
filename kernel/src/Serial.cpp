#include "Serial.h"
#include "IO.h"
#include "StringUtilities.h"

static const uint16_t PORT = 0xe9;

void Serial::Print(const char* string, const char* end)
{
    outb(PORT, (uint8_t*)string, String::Length(string));
    outb(PORT, (uint8_t*)end, String::Length(end));
}