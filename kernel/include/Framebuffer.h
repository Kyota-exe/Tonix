#pragma once

#include "Colour.h"

class Framebuffer
{
public:
    static void PlotPixel(unsigned int x, unsigned int y, Colour colour);
    static void TranslateVertical(long deltaY, const Colour& fillColour);
    static void Initialize();
    static long Width();
    static long Height();
private:
    static uint32_t* virtAddr;
    static uint16_t width;
    static uint16_t height;
    static uint8_t redShift;
    static uint8_t blueShift;
    static uint8_t greenShift;
};