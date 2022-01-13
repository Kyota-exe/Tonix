#include "Bitmap.h"
#include "Serial.h"

bool Bitmap::GetBit(uint64_t index) const
{
    uint64_t byteIndex = index / 8;

    if (byteIndex > size - 1)
    {
        Serial::Printf("Bitfield: Index %x exceeds bitmap bounds.", index);
        while (true) asm("hlt");
    }

    uint8_t bitIndex = index % 8;
    return (buffer[byteIndex] & (1 << bitIndex));
}

void Bitmap::SetBit(uint64_t index, bool value) const
{
    uint64_t byteIndex = index / 8;

    if (byteIndex > size - 1)
    {
        Serial::Printf("Bitfield: Index %x exceeds bitmap bounds.", index);
        while (true) asm("hlt");
    }

    uint8_t bitIndex = index % 8;
    if (value)
    {
        buffer[byteIndex] |= (1 << bitIndex);
    }
    else
    {
        buffer[byteIndex] &= ~(1 << bitIndex);
    }
}
