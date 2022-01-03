#ifndef MISKOS_BITMAP_H
#define MISKOS_BITMAP_H

#include <stddef.h>
#include <stdint.h>

struct Bitmap
{
    uint8_t* buffer;
    uint64_t size;
    bool GetBit(uint64_t index);
    void SetBit(uint64_t index, bool value);
};

#endif
