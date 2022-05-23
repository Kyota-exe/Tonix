#include "TextRenderer.h"
#include "Framebuffer.h"

const char* FONT_PATH = "/fonts/ibm/iv9x16u.psfu";

TextRenderer::TextRenderer() : font(new PSF2(String(FONT_PATH))) {}

TextRenderer::~TextRenderer()
{
    delete font;
}

void TextRenderer::Print(char c, long x, long y, const Colour& colour, const Colour& bgColour)
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

void TextRenderer::Paint(long x, long y, const Colour& colour)
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

void TextRenderer::ScrollDown(const Colour& fillColour)
{
    Framebuffer::TranslateVertical(font->Height(), fillColour);
}

WindowSize TextRenderer::GetWindowSize()
{
    return {
        .rowCount = static_cast<unsigned short>(Framebuffer::Height() / font->Height()),
        .columnCount = static_cast<unsigned short>(Framebuffer::Width() / font->Width()),
        .width = static_cast<unsigned short>(Framebuffer::Width()),
        .height = static_cast<unsigned short>(Framebuffer::Height()),
    };
}
