#include "PSF2Font.h"
#include "VFS.h"
#include "Serial.h"
#include <stdint.h>

const uint8_t PSF2_MAGIC_0 = 0x72;
const uint8_t PSF2_MAGIC_1 = 0xb5;
const uint8_t PSF2_MAGIC_2 = 0x4a;
const uint8_t PSF2_MAGIC_3 = 0x86;

PSF2Font::PSF2Font(const String &path)
{
    int fontFile = Open(path, 0);

    header = new PSF2Header;
    uint64_t headerSize = Read(fontFile, header, sizeof(PSF2Header));

    KAssert(header->magic[0] == PSF2_MAGIC_0, "Invalid PSF2 file.");
    KAssert(header->magic[1] == PSF2_MAGIC_1, "Invalid PSF2 file.");
    KAssert(header->magic[2] == PSF2_MAGIC_2, "Invalid PSF2 file.");
    KAssert(header->magic[3] == PSF2_MAGIC_3, "Invalid PSF2 file.");

    KAssert(headerSize == sizeof(PSF2Header), "Invalid PSF2 file: Could not read PSF2 header.");
    KAssert(headerSize == header->headerSize, "Header size does not match header size in PSF2 header.");

    Serial::Printf("Width: %d", header->width);
    Serial::Printf("Height: %d", header->height);
    Serial::Printf("Character size: %d", header->charSize);
    Serial::Printf("Glyph count: %d", header->glyphCount);
    Serial::Printf("Glyph buffer size: %d", header->glyphCount * header->charSize);

    uint64_t glyphBufferSize = header->glyphCount * header->charSize;
    glyphBuffer = new uint8_t[glyphBufferSize];
    Read(fontFile, glyphBuffer, glyphBufferSize);

    Close(fontFile);
}

Bitmap PSF2Font::GetGlyphBitmap(char c)
{
    Bitmap glyphBitmap;
    glyphBitmap.buffer = glyphBuffer + (c * header->charSize);
    glyphBitmap.size = header->charSize;

    return glyphBitmap;
}

uint32_t PSF2Font::Height()
{
    return header->height;
}

uint32_t PSF2Font::Width()
{
    return header->width;
}

PSF2Font::~PSF2Font()
{
    delete header;
}