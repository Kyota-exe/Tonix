#include "Bitmap.h"
#include "Serial.h"

bool Bitmap::GetBit(uint64_t index) const
{
    uint64_t byteIndex = index / 8;

    if (byteIndex > size - 1)
    {
        Serial::Printf("Bounds: %d", size);
        Serial::Printf("Bitfield: Index %d exceeds bitmap bounds.", byteIndex);
        while (true) asm("hlt");
    }

    uint8_t bitIndex = 1 << (index % 8);
    return (buffer[byteIndex] & bitIndex);
}

void Bitmap::SetBit(uint64_t index, bool value) const
{
    uint64_t byteIndex = index / 8;

    if (byteIndex > size - 1)
    {
        Serial::Printf("Bitfield: Index %x exceeds bitmap bounds.", index);
        while (true) asm("hlt");
    }

    uint8_t bitIndex = 1 << (index % 8);
    if (value)
    {
        buffer[byteIndex] |= bitIndex;
    }
    else
    {
        buffer[byteIndex] &= ~bitIndex;
    }
}
