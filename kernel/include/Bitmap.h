#pragma once

#include <stdint.h>

struct Bitmap
{
    uint8_t* buffer;
    uint64_t size;
    bool GetBit(uint64_t index) const;
    void SetBit(uint64_t index, bool value) const;
};
