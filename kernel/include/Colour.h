#pragma once

#include <stdint.h>

class Colour
{
public:
    Colour(uint8_t red, uint8_t green, uint8_t blue);
    static Colour FromANSICode(unsigned int code);
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
};