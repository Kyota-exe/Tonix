#include "Colour.h"
#include "Assert.h"

enum ANSICodes : unsigned int
{
    ForegroundBlack = 30,
    ForegroundRed = 31,
    ForegroundGreen = 32,
    ForegroundYellow = 33,
    ForegroundBlue = 34,
    ForegroundMagenta = 35,
    ForegroundCyan = 36,
    ForegroundWhite = 37,

    BackgroundBlack = 40,
    BackgroundRed = 41,
    BackgroundGreen = 42,
    BackgroundYellow = 43,
    BackgroundBlue = 44,
    BackgroundMagenta = 45,
    BackgroundCyan = 46,
    BackgroundWhite = 47
};

Colour Colour::FromANSICode(unsigned int code)
{
    switch (code)
    {
        case ForegroundBlack:
        case BackgroundBlack:
            return {0, 0, 0};

        case ForegroundRed:
        case BackgroundRed:
            return {255, 0, 0};

        case ForegroundGreen:
        case BackgroundGreen:
            return {0, 255, 0};

        case ForegroundYellow:
        case BackgroundYellow:
            return {255, 255, 0};

        case ForegroundBlue:
        case BackgroundBlue:
            return {0, 0, 255};

        case ForegroundMagenta:
        case BackgroundMagenta:
            return {255, 0, 255};

        case ForegroundCyan:
        case BackgroundCyan:
            return {0, 255, 255};

        case ForegroundWhite:
        case BackgroundWhite:
            return {255, 255, 255};

        default:
            Panic();
    }
}

Colour::Colour(uint8_t red, uint8_t green, uint8_t blue) : red(red), green(green), blue(blue) {}