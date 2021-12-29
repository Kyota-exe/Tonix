#include "SerialOutput.h"

SerialOutput::SerialOutput(uint16_t _port): port(_port) { }

void SerialOutput::Print(const char* string, const char* end)
{
    outb(port, (uint8_t*)string, StringLength(string));
    outb(port, (uint8_t*)end, StringLength(end));
}