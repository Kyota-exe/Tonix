#pragma once

#include "Colour.h"
#include "PSF2.h"

class TextRenderer
{
public:
    void Print(char c, long x, long y, Colour colour, Colour bgColour);
    void Paint(long x, long y, Colour colour);
    long CharsPerLine();
    long CharsPerColumn();
    TextRenderer();
    ~TextRenderer();
private:
    PSF2* font;
};