#pragma once

class PSF2Font;

#include "String.h"
#include "Bitmap.h"
#include "PSF2Glyph.h"

class PSF2Font
{
public:
    PSF2Glyph GetGlyphBitmap(char c) const;
    uint32_t Height() const;
    uint32_t Width() const;

    explicit PSF2Font(const String& path);
    ~PSF2Font();
    PSF2Font& operator=(const PSF2Font&) = delete;

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