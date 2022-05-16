#include "Bitmap.h"
#include "Assert.h"

Bitmap::Bitmap(uint8_t* buffer, uint64_t size, bool leastSignificantFirst) :
        buffer(buffer), size(size), leastSignificantFirst(leastSignificantFirst)
{
    Assert(buffer != nullptr && size > 0);
}

bool Bitmap::GetBit(uint64_t index) const
{
    uint64_t byteIndex = index / 8;
    Assert(byteIndex < size);

    uint8_t bitIndex = leastSignificantFirst ? 1 << (7 - (index % 8)) : 1 << (index % 8);
    return (buffer[byteIndex] & bitIndex);
}

void Bitmap::SetBit(uint64_t index, bool value) const
{
    uint64_t byteIndex = index / 8;
    Assert(byteIndex < size);

    uint8_t bitIndex = leastSignificantFirst ? 1 << (7 - index % 8) : 1 << (index % 8);
    if (value)
    {
        buffer[byteIndex] |= bitIndex;
    }
    else
    {
        buffer[byteIndex] &= ~bitIndex;
    }
}

uint64_t Bitmap::TotalNumberOfBits() const
{
    return size * 8;
}
