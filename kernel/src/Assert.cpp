#include "Assert.h"
#include "Serial.h"

[[noreturn]] void KernelPanic(const char* assertion, const char* file, unsigned int line, const char* function)
{
    asm volatile("cli");
    Serial::Log("! KERNEL PANIC !");

    Serial::Log("%s", assertion);
    Serial::Log("%s: line: %d", file, line);
    Serial::Log("Function: %s", function);

    while (true) asm("hlt");
}