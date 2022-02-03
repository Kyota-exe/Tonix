#include "TextRenderer.h"
#include "Framebuffer.h"
#include "Serial.h"

TextRenderer::TextRenderer(const String& fontPath, int characterSpacing) : characterSpacing(characterSpacing)
{
    font = new PSF2Font(fontPath);
}

void TextRenderer::Print(char c, unsigned int x, unsigned int y, Colour colour)
{
    PSF2Glyph glyph = font->GetGlyphBitmap(c);

    for (uint32_t glyphY = 0; glyphY < font->Height(); ++glyphY)
    {
        for (uint32_t glyphX = 0; glyphX < font->Width(); ++glyphX)
        {
            if (glyph.GetPixel(glyphX, glyphY))
            {
                unsigned int screenX = x + glyphX;
                unsigned int screenY = y + glyphY;
                Framebuffer::instance->PlotPixel(screenX, screenY, colour);
            }
        }
    }
}

void TextRenderer::Print(const String& string, unsigned int x, unsigned int y, Colour colour)
{
    unsigned int screenX = x;
    unsigned int screenY = y;

    for (const char c : string)
    {
        Print(c, screenX, screenY, colour);
        screenX += font->Width() + characterSpacing;
    }
}