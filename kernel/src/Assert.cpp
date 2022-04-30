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

void KernelWarn(const char* message, const char* file, unsigned int line)
{
    Serial::Log("\033[93m[WARNING] %s:%d -> %s\033[39m", file, line, message);
}