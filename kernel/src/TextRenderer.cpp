#include "TextRenderer.h"
#include "Framebuffer.h"

const char* FONT_PATH = "/fonts/Uni2-Terminus20x10.psf";

TextRenderer::TextRenderer() : font(new PSF2(String(FONT_PATH))) {}

TextRenderer::~TextRenderer()
{
    delete font;
}

void TextRenderer::Print(char c, long x, long y, Colour colour, Colour bgColour)
{
    PSF2Glyph glyph = font->GetGlyphBitmap(c);
    for (uint32_t glyphY = 0; glyphY < font->Height(); ++glyphY)
    {
        for (uint32_t glyphX = 0; glyphX < font->Width(); ++glyphX)
        {
            long screenX = x * font->Width() + glyphX;
            long screenY = y * font->Height() + glyphY;
            Framebuffer::PlotPixel(screenX, screenY, glyph.GetPixel(glyphX, glyphY) ? colour : bgColour);
        }
    }
}

void TextRenderer::Paint(long x, long y, Colour colour)
{
    for (uint32_t glyphY = 0; glyphY < font->Height(); ++glyphY)
    {
        for (uint32_t glyphX = 0; glyphX < font->Width(); ++glyphX)
        {
            long screenX = x * font->Width() + glyphX;
            long screenY = y * font->Height() + glyphY;
            Framebuffer::PlotPixel(screenX, screenY, colour);
        }
    }
}

long TextRenderer::CharsPerLine()
{
    Assert(Framebuffer::Width() / font->Width() == 102);
    return Framebuffer::Width() / font->Width();
}

long TextRenderer::CharsPerColumn()
{
    Assert(Framebuffer::Height() / font->Height() == 38);
    return Framebuffer::Height() / font->Height();
}