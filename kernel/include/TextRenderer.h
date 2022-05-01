#pragma once

#include "Colour.h"
#include "PSF2.h"

class TextRenderer
{
public:
    void Print(char c, long x, long y, const Colour& colour, const Colour& bgColour);
    void Paint(long x, long y, const Colour& colour);
    void ScrollDown(const Colour& fillColour);
    long CharsPerLine();
    long CharsPerColumn();
    TextRenderer();
    ~TextRenderer();
private:
    PSF2* font;
};