#pragma once

#include <stdint.h>

class Colour
{
public:
    Colour(uint8_t _red, uint8_t _green, uint8_t _blue);
    Colour() = default;
    Colour(const Colour& original);
    Colour& operator=(const Colour& original);
    static Colour FromANSICode(unsigned int code);
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};