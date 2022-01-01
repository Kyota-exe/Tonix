#include "Bitmap.h"

bool Bitmap::GetBit(uint64_t index)
{
    uint64_t byteIndex = index / 8;
    uint8_t bitIndex = index % 8;
    return (buffer[byteIndex] & (1 << bitIndex));
}

void Bitmap::SetBit(uint64_t index, bool value)
{
    uint64_t byteIndex = index / 8;
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
