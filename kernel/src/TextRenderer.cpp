#include "TextRenderer.h"
#include "Framebuffer.h"
#include "Serial.h"
#include "Assert.h"

TextRenderer::TextRenderer(const String& fontPath, Colour textColour, int characterSpacing, Colour backgroundColour) :
                           characterSpacing(characterSpacing), backgroundColour(backgroundColour), textColour(textColour)
{
    font = new PSF2Font(fontPath);
    cursorX = 0;
    cursorY = 0;
}

void TextRenderer::Print(char c, long x, long y, Colour colour)
{
    PSF2Glyph glyph = font->GetGlyphBitmap(c);

    for (uint32_t glyphY = 0; glyphY < font->Height(); ++glyphY)
    {
        for (uint32_t glyphX = 0; glyphX < font->Width(); ++glyphX)
        {
            unsigned int screenX = x + glyphX;
            unsigned int screenY = y + glyphY;
            Framebuffer::PlotPixel(screenX, screenY,glyph.GetPixel(glyphX, glyphY) ? colour : backgroundColour);
        }
    }
}

void TextRenderer::Print(const String& string)
{
    uint64_t currentIndex = 0;
    while (currentIndex < string.GetLength())
    {
        char c = string[currentIndex++];

        switch (c)
        {
            case '\n': // Line Feed
            {
                cursorY += font->Height();
                cursorX = 0;
                break;
            }
            case '\b': // Backspace
            {
                cursorX -= font->Width();
                break;
            }
            case '\t': // Horizontal Tab
            {
                cursorX += font->Width() * 4;
                break;
            }
            case '\033': // Escape
            {
                if (string.Match(currentIndex, '[')) // Control Sequence Introducer
                {
                    currentIndex++;
                    if (string.Match(currentIndex, 'H'))
                    {
                        cursorX = 0;
                        cursorY = 0;
                        currentIndex++;
                    }
                    else if (string.Match(currentIndex, '6'))
                    {
                        if (string.Match(currentIndex + 1, 'n'))
                        {
                            currentIndex += 2;
                            Assert(false);
                        }
                    }
                    else if (string.Match(currentIndex, 's'))
                    {
                        currentIndex++;
                        Assert(false);
                    }
                    else if (string.Match(currentIndex, 'u'))
                    {
                        currentIndex++;
                        Assert(false);
                    }
                    else if (string.Match(currentIndex, 'J'))
                    {
                        currentIndex++;
                        Assert(false);
                    }
                    else if (string.Match(currentIndex, 'K'))
                    {
                        currentIndex++;
                        Assert(false);
                    }
                    else if (string.Match(currentIndex, '='))
                    {
                        currentIndex++;
                        Assert(false);
                    }
                    else if (string.Match(currentIndex, '?'))
                    {
                        currentIndex++;
                        Assert(false);
                    }
                    else
                    {
                        String numberString;
                        while (string.IsNumeric(currentIndex))
                        {
                            numberString.Push(string[currentIndex++]);
                        }
                        auto number = numberString.ToUnsignedInt();

                        if (string.Match(currentIndex, 'A'))
                        {
                            cursorY -= font->Height() * number;
                            currentIndex++;
                        }
                        else if (string.Match(currentIndex, 'B'))
                        {
                            cursorY += font->Height() * number;
                            currentIndex++;
                        }
                        else if (string.Match(currentIndex, 'C'))
                        {
                            cursorX += font->Width() * number;
                            currentIndex++;
                        }
                        else if (string.Match(currentIndex, 'D'))
                        {
                            cursorX -= font->Width() * number;
                            currentIndex++;
                        }
                        else if (string.Match(currentIndex, 'E'))
                        {
                            cursorY += font->Height() * number;
                            cursorX = 0;
                            currentIndex++;
                        }
                        else if (string.Match(currentIndex, 'F'))
                        {
                            cursorY -= font->Height() * number;
                            cursorX = 0;
                            currentIndex++;
                        }
                        else if (string.Match(currentIndex, 'G'))
                        {
                            cursorX = font->Width() * number;
                            currentIndex++;
                        }
                        else if (string.Match(currentIndex, 'J'))
                        {
                            currentIndex++;
                            Assert(false);
                        }
                        else if (string.Match(currentIndex, 'K'))
                        {
                            currentIndex++;
                            Assert(false);
                        }
                        else if (string.Match(currentIndex, 'm'))
                        {
                            currentIndex++;
                            Assert(false);
                        }
                        else if (string.Match(currentIndex, ';'))
                        {
                            currentIndex++;
                            Assert(false);
                        }
                    }
                }
                else if (string.Match(currentIndex, 'M'))
                {
                    cursorY -= font->Height();
                    currentIndex++;
                }
                else if (string.Match(currentIndex, '7'))
                {
                    currentIndex++;
                    Assert(false);
                }
                else if (string.Match(currentIndex, '8'))
                {
                    currentIndex++;
                    Assert(false);
                }

                break;
            }

            default:
            {
                Print(c, cursorX, cursorY, textColour);
                cursorX += font->Width() + characterSpacing;
            }
        }
    }

    Assert(cursorX >= 0);
    Assert(cursorY >= 0);
}