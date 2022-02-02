#pragma once

#include "String.h"
#include "Bitmap.h"

class PSF2Font
{
public:
    Bitmap GetGlyphBitmap(char c);
    uint32_t Height();
    uint32_t Width();

    explicit PSF2Font(const String& path);
    ~PSF2Font();

private:
    struct PSF2Header
    {
        uint8_t magic[4];
        uint32_t version;
        uint32_t headerSize;
        uint32_t flags;
        uint32_t glyphCount;
        uint32_t charSize;
        uint32_t height;
        uint32_t width;
    } __attribute__((packed));

    PSF2Header* header;
    uint8_t* glyphBuffer;
};