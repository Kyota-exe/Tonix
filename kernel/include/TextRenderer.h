#pragma once

#include "Colour.h"
#include "PSF2Font.h"

class TextRenderer
{
public:
    void Print(char c, unsigned int x, unsigned int y, Colour colour);
    void Print(const String& string, unsigned int& x, unsigned int& y, Colour colour);
    explicit TextRenderer(const String& fontPath, int characterSpacing = 0);
    int characterSpacing;
private:
    PSF2Font* font;
};