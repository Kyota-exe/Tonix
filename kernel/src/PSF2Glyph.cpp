#include "PSF2Glyph.h"

PSF2Glyph::PSF2Glyph(uint8_t* glyphBuffer, uint32_t size_, PSF2Font* psf2Font)
{
    buffer = glyphBuffer;
    size = size_;
    firstIsLeft = true;
    font = psf2Font;
}

bool PSF2Glyph::GetPixel(unsigned int x, unsigned int y)
{
    uint64_t padding = font->Width() % 8;
    uint64_t accumulatedPadding = padding == 0 ? 0 : (8 - padding) * y;
    uint64_t index = y * font->Width() + x + accumulatedPadding;
    return GetBit(index);
}