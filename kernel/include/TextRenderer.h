#pragma once

#include "Colour.h"
#include "PSF2.h"

class TextRenderer
{
public:
    void Print(char c, long x, long y, Colour colour, Colour bgColour);
    void Paint(long x, long y, Colour colour);
    unsigned int FontWidth();
    unsigned int FontHeight();
    long ScreenWidth();
    long ScreenHeight();
    TextRenderer();
    ~TextRenderer();
private:
    PSF2* font;
};