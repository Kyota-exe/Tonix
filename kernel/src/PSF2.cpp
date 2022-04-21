#include "PSF2.h"
#include "VFS.h"
#include "Serial.h"
#include "Heap.h"
#include <stdint.h>

constexpr uint8_t PSF2_MAGIC_0 = 0x72;
constexpr uint8_t PSF2_MAGIC_1 = 0xb5;
constexpr uint8_t PSF2_MAGIC_2 = 0x4a;
constexpr uint8_t PSF2_MAGIC_3 = 0x86;
constexpr uint32_t PSF2_HEADER_FLAG_HAS_UNICODE_TABLE = 0x01;

PSF2::PSF2(const String &path)
{
    int fontFile = VFS::kernelVfs->Open(path, 0);

    header = new PSF2::Header;
    uint64_t headerSize = VFS::kernelVfs->Read(fontFile, header, sizeof(PSF2::Header));

    Assert(header->magic[0] == PSF2_MAGIC_0 &&
           header->magic[1] == PSF2_MAGIC_1 &&
           header->magic[2] == PSF2_MAGIC_2 &&
           header->magic[3] == PSF2_MAGIC_3);

    Assert(headerSize == sizeof(PSF2::Header) && headerSize == header->headerSize);

    Serial::Printf("Width: %d", header->width);
    Serial::Printf("Height: %d", header->height);
    Serial::Printf("Character size: %d", header->charSize);
    Serial::Printf("Glyph count: %d", header->glyphCount);
    Serial::Printf("Glyph buffer size: %d", header->glyphCount * header->charSize);
    Serial::Printf("Has unicode table: %d", (bool)(header->flags & PSF2_HEADER_FLAG_HAS_UNICODE_TABLE));
    Serial::Printf("Header size: %d", header->headerSize);

    uint64_t glyphBufferSize = header->glyphCount * header->charSize;
    glyphBuffer = new (Allocator::Permanent) uint8_t[glyphBufferSize];
    VFS::kernelVfs->Read(fontFile, glyphBuffer, glyphBufferSize);

    VFS::kernelVfs->Close(fontFile);
}

PSF2Glyph PSF2::GetGlyphBitmap(char c) const
{
    return {glyphBuffer + (c * header->charSize), header->charSize, Width(), Height()};
}

uint32_t PSF2::Height() const
{
    return header->height;
}

uint32_t PSF2::Width() const
{
    return header->width;
}

PSF2::~PSF2()
{
    Panic();
    delete header;
    delete[] glyphBuffer;
}