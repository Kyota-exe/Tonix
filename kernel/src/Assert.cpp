#include "Assert.h"
#include "Serial.h"

[[noreturn]] void KernelPanic(const char* assertion, const char* file, unsigned int line, const char* function)
{
    asm volatile("cli");
    Serial::Print("! KERNEL PANIC !");

    Serial::Printf("%s", assertion);
    Serial::Printf("%s: line: %d", file, line);
    Serial::Printf("Function: %s", function);

    while (true) asm("hlt");
}