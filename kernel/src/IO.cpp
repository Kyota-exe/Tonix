#include "IO.h"

inline void outb(uint16_t port, uint8_t value)
{
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

void outb(uint16_t port, uint8_t* values, size_t size)
{
    for (size_t i = 0; i < size; ++i)
    {
        outb(port, *values++);
    }
}