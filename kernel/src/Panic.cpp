#include "Panic.h"
#include "Serial.h"

void Panic(const char* message)
{
    Serial::Print("KERNEL PANIC!");
    Serial::Print(message);
    Serial::Print("Hanging...");
    while (true) asm("cli\n hlt\n");
}

void Panic(const char* message, const char* file, unsigned int line, const char* function)
{
    // TODO: Don't print strings directly
    Serial::Print("KERNEL PANIC!");
    Serial::Print(message);
    Serial::Print(file, "");
    Serial::Printf(": line %d", line);
    Serial::Print("Function: ", "");
    Serial::Print(function);
    Serial::Print("Hanging...");
    while (true) asm("cli\n hlt\n");
}