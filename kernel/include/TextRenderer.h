#pragma once

#include "Colour.h"
#include "PSF2Font.h"

template <typename T>
struct Vector;

class TextRenderer
{
public:
    void Print(char c, long x, long y, Colour colour);
    void Erase(long x, long y);
    void ProcessEscapeSequence(char command, bool hasCSI, const Vector<unsigned int>& arguments);
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