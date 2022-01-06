#include "PIC.h"

void InitializePIC()
{
    Serial::Print("Initializing the 8259 PIC...");

    // ICW1
    // 0x01 bit: ICW4 has to be read
    // 0x10 bit: Initialize PIC
    outb(0x20, 0x11);
    io_wait();
    outb(0xa0, 0x11);
    io_wait();

    // ICW2
    // IRQ start in IDT
    outb(0x21, 0x20);
    io_wait();
    outb(0xa1, 0x28);
    io_wait();

    // ICW3
    outb(0x21, 0b100);
    io_wait();
    outb(0xa1, 0b010);
    io_wait();

    // ICW4
    outb(0x21, 0x01);
    io_wait();
    outb(0xa1, 0x01);
    io_wait();

    // Mask all PIC IRQs by default
    outb(0x21, 0xff);
    io_wait();
    outb(0xa1, 0xff);
    io_wait();

    Serial::Print("Completed initialization of the 8259 PIC.", "\n\n");
}

void PICSendEIO(int irq)
{
    // 0x20: EIO
    outb(irq < 8 ? 0x20 : 0xa0, 0x20);
}

void ActivatePICKeyboardInterrupts()
{
    outb(0x21, inb(0x21) & ~0b00000010);
}