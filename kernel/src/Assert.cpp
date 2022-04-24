#include "Assert.h"
#include "Serial.h"

[[noreturn]] void KernelPanic(const char* assertion, const char* file, unsigned int line, const char* function)
{
    asm volatile("cli");
    Serial::Print("! KERNEL PANIC !");

    // TODO: Don't print strings directly
    Serial::Print(assertion);
    Serial::Print(file, "");
    Serial::Printf(": line %d", line);
    Serial::Print("Function: ", "");
    Serial::Print(function);

    while (true) asm("hlt");
}