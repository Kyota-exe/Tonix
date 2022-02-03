#pragma once

class PSF2Glyph;

#include "Bitmap.h"
#include "PSF2Font.h"

class PSF2Glyph : Bitmap
{
public:
    bool GetPixel(unsigned int x, unsigned int y);
    PSF2Glyph(uint8_t* glyphBuffer, uint32_t size_, PSF2Font* psf2Font);
private:
    PSF2Font* font;
};