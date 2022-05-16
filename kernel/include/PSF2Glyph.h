#pragma once

#include "Bitmap.h"

class PSF2Glyph
{
public:
    bool GetPixel(unsigned int x, unsigned int y) const;
    PSF2Glyph(uint8_t* buffer, uint32_t size, unsigned int width, unsigned int height);
private:
    Bitmap bitmap;
    unsigned int width;
    unsigned int height;
};