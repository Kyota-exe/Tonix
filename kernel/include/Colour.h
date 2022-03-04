#pragma once

#include <stdint.h>

class Colour
{
public:
    Colour(uint8_t _red, uint8_t _green, uint8_t _blue);
    static Colour FromANSICode(unsigned int code);
    uint8_t red;
    uint8_t green;
    uint8_t blue;

    enum class ANSICodes
    {
        FGBlack = 30,
        FGRed = 31,
        FGGreen = 32,
        FGYellow = 33,
        FGBlue = 34,
        FGMagenta = 35,
        FGCyan = 36,
        FGWhite = 37,
        FGDefault = 39,

        BGBlack = 40,
        BGRed = 41,
        BGGreen = 42,
        BGYellow = 43,
        BGBlue = 44,
        BGMagenta = 45,
        BGCyan = 46,
        BGWhite = 47,
        BGDefault = 49

    };
};