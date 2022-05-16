#pragma once

#include <stdint.h>

class Bitmap
{
public:
    Bitmap(uint8_t* buffer, uint64_t size, bool leastSignificantFirst);
    bool GetBit(uint64_t index) const;
    void SetBit(uint64_t index, bool value) const;
    uint64_t TotalNumberOfBits() const;
private:
    uint8_t* buffer;
    uint64_t size;
    bool leastSignificantFirst = false;
};
