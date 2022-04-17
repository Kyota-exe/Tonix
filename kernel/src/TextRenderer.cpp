#include "TextRenderer.h"
#include "Framebuffer.h"
#include "Serial.h"
#include "Assert.h"
#include "Vector.h"

TextRenderer::TextRenderer(const String& fontPath, Colour textColour, Colour textBackgroundColour,
                           Colour backgroundColour, int characterSpacing) :
                           characterSpacing(characterSpacing), backgroundColour(backgroundColour),
                           textColour(textColour), textBgColour(textBackgroundColour)

{
    font = new PSF2Font(fontPath);
    cursorX = 0;
    cursorY = 0;
}

void TextRenderer::Print(char c, long x, long y, Colour colour, Colour bgColour)
{
    PSF2Glyph glyph = font->GetGlyphBitmap(c);

    for (uint32_t glyphY = 0; glyphY < font->Height(); ++glyphY)
    {
        for (uint32_t glyphX = 0; glyphX < font->Width(); ++glyphX)
        {
            unsigned int screenX = x + glyphX;
            unsigned int screenY = y + glyphY;
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
            unsigned int screenX = x + glyphX;
            unsigned int screenY = y + glyphY;
            Framebuffer::PlotPixel(screenX, screenY, colour);
        }
    }
}

void TextRenderer::ProcessEscapeSequence(char command, bool hasCSI, const Vector<unsigned int>& arguments)
{
    uint64_t argCount = arguments.GetLength();

    // Temporary printing of escape sequences
    {
        Serial::Print("ESCAPE--------------------------------------------------");
        Serial::Print("Command: ", "");
        char* commandBuffer = new char[2];
        commandBuffer[0] = command; commandBuffer[1] = 0;
        Serial::Print(commandBuffer);

        Serial::Printf("hasCSI: %d", hasCSI);

        Serial::Printf("Arguments (%d): ", arguments.GetLength());
        for (const unsigned int argument : arguments)
        {
            Serial::Printf("%d ", argument);
        }
    }

    if (hasCSI)
    {
        switch (command)
        {
            case 'H':
                if (argCount == 0) { cursorX = cursorY = 0; break; }
                Assert(argCount == 2);
                cursorY = arguments.Get(0) * font->Height();
                cursorX = arguments.Get(1) * font->Width();
                break;
            case 'f':
                Assert(argCount == 2);
                cursorY = arguments.Get(0) * font->Height();
                cursorX = arguments.Get(1) * font->Width();
                break;
            case 'A':
                Assert(argCount == 1);
                cursorY -= arguments.Get(0) * font->Height();
                break;
            case 'B':
                Assert(argCount == 1);
                cursorY += arguments.Get(0) * font->Height();
                break;
            case 'C':
                Assert(argCount == 1);
                cursorX += arguments.Get(0) * font->Width();
                break;
            case 'D':
                Assert(argCount == 1);
                cursorX -= arguments.Get(0) * font->Width();
                break;
            case 'E':
                Assert(argCount == 1);
                cursorY += arguments.Get(0) * font->Height();
                cursorX = 0;
                break;
            case 'F':
                Assert(argCount == 1);
                cursorY -= arguments.Get(0) * font->Height();
                cursorX = 0;
                break;
            case 'G':
                Assert(argCount == 1);
                cursorX = arguments.Get(0) * font->Width();
                break;
            case 'm':
                for (unsigned int arg : arguments)
                {
                    switch (arg)
                    {
                        case 30 ... 39:
                            textColour = Colour::FromANSICode(arg);
                            break;
                        case 40 ... 49:
                            backgroundColour = Colour::FromANSICode(arg);
                            break;
                        default:
                            Panic();
                    }
                }
                break;
            default:
                Panic();
        }
    }
    else
    {
        switch (command)
        {
            case 'M':
                cursorY -= font->Height();
                break;
            default:
                Panic();
        }
    }
}

void TextRenderer::Print(const String& string)
{
    // Erase previous cursor
    Paint(cursorX, cursorY, backgroundColour);

    uint64_t currentIndex = 0;
    while (currentIndex < string.GetLength())
    {
        char c = string[currentIndex++];

        bool eraseCharacter = false;

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
                eraseCharacter = true;
                break;
            }
            case '\t': // Horizontal Tab
            {
                cursorX += font->Width() * 4;
                break;
            }
            case '\a': // Terminal Bell
            {
                Panic();
            }
            case '\033': // Escape
            {
                bool hasCSI = string.Match(currentIndex, '[');
                if (hasCSI) currentIndex++;

                Vector<unsigned int> arguments;
                do
                {
                    String numberString;
                    while (string.IsNumeric(currentIndex))
                    {
                        numberString.Push(string[currentIndex++]);
                    }

                    Assert(numberString.GetLength() > 0);
                    arguments.Push(numberString.ToUnsignedInt());

                } while (string.Match(currentIndex++, ';'));

                currentIndex--;

                Assert(currentIndex < string.GetLength());
                char command = string[currentIndex++];

                ProcessEscapeSequence(command, hasCSI, arguments);
                break;
            }
            default:
            {
                Print(c, cursorX, cursorY, textColour, textBgColour);
                cursorX += font->Width() + characterSpacing;
            }
        }

        Assert(cursorX >= 0);
        Assert(cursorY >= 0);
        Assert(cursorY < Framebuffer::Height());

        if (eraseCharacter)
        {
            Paint(cursorX, cursorY, backgroundColour);
        }
    }

    // Render cursor
    Paint(cursorX, cursorY, textColour);
}