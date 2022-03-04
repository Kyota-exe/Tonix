#include "PIC.h"

constexpr uint16_t MASTER_COMMAND = 0x20;
constexpr uint16_t SLAVE_COMMAND = 0xa0;
constexpr uint16_t MASTER_DATA = MASTER_COMMAND + 1;
constexpr uint16_t SLAVE_DATA = SLAVE_COMMAND + 1;

void InitializePIC()
{
    // ICW1
    // 0x01 bit: ICW4 has to be read
    // 0x10 bit: Initialize PIC
    outb(MASTER_COMMAND, 0x11);
    outb(SLAVE_COMMAND, 0x11);

    // ICW2
    // IRQ start in IDT
    outb(MASTER_DATA, 0x20);
    outb(SLAVE_DATA, 0x28);

    // ICW3
    outb(MASTER_DATA, 0b100);
    outb(SLAVE_DATA, 0b010);

    // ICW4
    outb(MASTER_DATA, 0x01);
    outb(SLAVE_DATA, 0x01);

    // Mask all PIC IRQs by default
    outb(MASTER_DATA, 0xff);
    outb(SLAVE_DATA, 0xff);
}

void PICSendEIO(int irq)
{
    // 0x20: EIO
    outb(irq < 8 ? MASTER_COMMAND : SLAVE_COMMAND, 0x20);
}

void ActivatePICKeyboardInterrupts()
{
    outb(MASTER_DATA, inb(MASTER_DATA) & ~0b00000010);
}