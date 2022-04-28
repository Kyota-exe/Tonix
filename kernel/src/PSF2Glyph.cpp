#include "PSF2Glyph.h"

PSF2Glyph::PSF2Glyph(uint8_t* buffer, uint32_t size, unsigned int width, unsigned int height) :
    bitmap(buffer, size, true), width(width), height(height) {}

bool PSF2Glyph::GetPixel(unsigned int x, unsigned int y) const
{
    uint64_t padding = width % 8;
    uint64_t accumulatedPadding = padding == 0 ? 0 : (8 - padding) * y;
    uint64_t index = y * width + x + accumulatedPadding;
    return bitmap.GetBit(index);
}