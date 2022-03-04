#include "Bitmap.h"
#include "Assert.h"

bool Bitmap::GetBit(uint64_t index) const
{
    uint64_t byteIndex = index / 8;
    Assert(byteIndex < size);

    uint8_t bitIndex = firstIsLeft ? 1 << (7 - (index % 8)) : 1 << (index % 8);
    return (buffer[byteIndex] & bitIndex);
}

void Bitmap::SetBit(uint64_t index, bool value) const
{
    uint64_t byteIndex = index / 8;
    Assert(byteIndex < size);

    uint8_t bitIndex = firstIsLeft ? 1 << (7 - index % 8) : 1 << (index % 8);
    if (value)
    {
        buffer[byteIndex] |= bitIndex;
    }
    else
    {
        buffer[byteIndex] &= ~bitIndex;
    }
}
