#include "PIT.h"

const uint16_t PIT_MODE_COMMAND = 0x43;
const uint16_t PIT_CHANNEL0_DATA = 0x40;
const uint64_t PIT_BASE_FREQUENCY = 1193182; // 1.1931816666 MHz

uint16_t PITGetTick()
{
    outb(PIT_MODE_COMMAND, 0);
    uint8_t low = inb(PIT_CHANNEL0_DATA);
    uint8_t high = inb(PIT_CHANNEL0_DATA);
    return ((uint16_t)high << 8) | low;
}

void PITSetReloadValue(uint16_t reloadValue)
{
    // From most to least significant bits: Channel 0, lobye/hibyte, rate generator, 16-bit binary
    outb(PIT_MODE_COMMAND, 0b00'11'010'0);

    outb(PIT_CHANNEL0_DATA, reloadValue);
    outb(PIT_CHANNEL0_DATA, reloadValue >> 8);
}

void PITSetFrequency(uint64_t frequency)
{
    uint64_t reloadValue = PIT_BASE_FREQUENCY / frequency;
    // Round up if the mantissa is greater than 0.5
    if (PIT_BASE_FREQUENCY % frequency > frequency / 2) reloadValue++;
    PITSetReloadValue(reloadValue);
}