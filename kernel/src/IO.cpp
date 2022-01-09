#include "IO.h"

void outb(uint16_t port, uint8_t value)
{
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

uint8_t inb(uint16_t port)
{
    uint8_t value;
    asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

void outb(uint16_t port, uint8_t* values, uint64_t size)
{
    for (uint64_t i = 0; i < size; ++i)
    {
        outb(port, *values++);
    }
}

void io_wait()
{
    outb(0x80, 0);
}