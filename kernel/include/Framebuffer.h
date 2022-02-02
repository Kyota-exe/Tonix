#pragma once

#include "Colour.h"

class Framebuffer
{
public:
    void PlotPixel(unsigned int x, unsigned int y, Colour colour);
    static void Initialize();
    static Framebuffer* instance;
private:
    uint32_t* virtAddr;
    uint16_t width;
    uint16_t height;
    uint8_t redShift;
    uint8_t blueShift;
    uint8_t greenShift;
    Framebuffer() = default;
};