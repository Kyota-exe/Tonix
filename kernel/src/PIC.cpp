#include "PIC.h"

void InitializePIC()
{
    Serial::Print("Initializing the 8259 PIC...");

    // ICW1
    // 0x01 bit: ICW4 has to be read
    // 0x10 bit: Initialize PIC
    outb(0x20, 0x11);
    outb(0xa0, 0x11);

    // ICW2
    // IRQ start in IDT
    outb(0x21, 0x20);
    outb(0xa1, 0x28);

    // ICW3
    outb(0x21, 0b100);
    outb(0xa1, 0b010);

    // ICW4
    outb(0x21, 0x01);
    outb(0xa1, 0x01);

    // Mask all PIC IRQs by default
    outb(0x21, 0xff);
    outb(0xa1, 0xff);

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