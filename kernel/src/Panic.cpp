#include "Panic.h"
#include "Serial.h"

void Panic(const char* message)
{
    Serial::Print("KERNEL PANIC: ", "");
    Serial::Print(message);
    Serial::Print("Hanging...");
    while (true) asm("cli\n hlt\n");
}

void KAssert(bool expression, const char* message)
{
    if (!expression)
    {
        Panic(message);
    }
}