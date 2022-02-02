#include "TextRenderer.h"
#include "Framebuffer.h"
#include "Serial.h"

PSF2Font* TextRenderer::font = nullptr;

TextRenderer::TextRenderer(const String& fontPath)
{
    if (font == nullptr)
    {
        font = new PSF2Font(fontPath);
    }
}

void TextRenderer::Print(char c, unsigned int x, unsigned int y, Colour colour)
{
    Bitmap glyphBitmap = font->GetGlyphBitmap(c);

    for (uint32_t glyphY = 0; glyphY < font->Height(); ++glyphY)
    {
        for (uint32_t glyphX = font->Width() - 1; glyphX < font->Width(); --glyphX)
        {
            uint64_t index = (glyphY * font->Width()) + glyphX;
            if (glyphBitmap.GetBit(index))
            {
                unsigned int screenX = x + font->Width() - glyphX;
                unsigned int screenY = y + glyphY;
                Framebuffer::instance->PlotPixel(screenX, screenY, colour);
            }

            if (glyphBitmap.GetBit(glyphY * font->Width() + glyphX))
                Serial::Print("#", "");
            else Serial::Print(" ", "");
        }
        Serial::Print("");
    }
}

void TextRenderer::Print(const String& string, unsigned int x, unsigned int y, Colour colour)
{
    unsigned int screenX = x;
    unsigned int screenY = y;

    for (const char c : string)
    {
        Print(c, screenX, screenY, colour);
        screenX += font->Width();
    }
}