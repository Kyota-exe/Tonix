#include "Colour.h"
#include "Assert.h"

Colour::Colour(uint8_t _red, uint8_t _green, uint8_t _blue) : red(_red), green(_green), blue(_blue) {}

Colour Colour::FromANSICode(unsigned int code)
{
    switch (code)
    {
        case 30: // Black Foreground
        case 40: // Black Background
        case 49: // Default Background
            return {0, 0, 0};

        case 31: // Red Foreground
        case 41: // Red Background
            return {255, 0, 0};

        case 32: // Green Foreground
        case 42: // Green Background
            return {0, 255, 0};

        case 33: // Yellow Foreground
        case 43: // Yellow Background
            return {255, 255, 0};

        case 34: // Blue Foreground
        case 44: // Blue Background
            return {0, 0, 255};

        case 35: // Magenta Foreground
        case 45: // Magenta Background
            return {255, 0, 255};

        case 36: // Cyan Foreground
        case 46: // Cyan Background
            return {0, 255, 255};

        case 37: // White Foreground
        case 47: // White Background
        case 39: // Default Foreground
            return {255, 255, 255};

        default:
            Panic();
    }
}