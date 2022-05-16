#pragma once

class PSF2;

#include "String.h"
#include "Bitmap.h"
#include "PSF2Glyph.h"

class PSF2
{
public:
    PSF2Glyph GetGlyphBitmap(char c) const;
    uint32_t Height() const;
    uint32_t Width() const;

    explicit PSF2(const String& path);
    ~PSF2();
    PSF2& operator=(const PSF2&) = delete;

private:
    struct Header;
    Header* header;
    uint8_t* glyphBuffer;
};

struct PSF2::Header
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