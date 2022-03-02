#pragma once

#include "Colour.h"
#include "PSF2Font.h"

class TextRenderer
{
public:
    void Print(char c, long x, long y, Colour colour);
    void Print(const String& string);
    explicit TextRenderer(const String& fontPath, Colour textColour, int characterSpacing, Colour backgroundColour);
    int characterSpacing;
    Colour backgroundColour;
    Colour textColour;
private:
    long cursorX;
    long cursorY;
    PSF2Font* font;
};