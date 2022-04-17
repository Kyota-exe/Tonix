#pragma once

#include "Colour.h"
#include "PSF2Font.h"
#include "Vector.h"

class TextRenderer
{
public:
    void Print(char c, long x, long y, Colour colour, Colour bgColour);
    void Paint(long x, long y, Colour colour);
    void ProcessEscapeSequence(char command, bool hasCSI, const Vector<unsigned int>& arguments);
    void Print(const String& string);
    TextRenderer(const String& fontPath, Colour textColour,
                 Colour textBackgroundColour, Colour backgroundColour, int characterSpacing);
    int characterSpacing;
    Colour backgroundColour;
    Colour textColour;
    Colour textBgColour;
private:
    long cursorX;
    long cursorY;
    PSF2Font* font;
};