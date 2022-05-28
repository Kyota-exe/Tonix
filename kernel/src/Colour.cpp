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
            return {0x26, 0x2c, 0x37};

        case ForegroundRed:
        case BackgroundRed:
            return {0xe0, 0x6c, 0x75};

        case ForegroundGreen:
        case BackgroundGreen:
            return {0x99, 0xcd, 0x73};

        case ForegroundYellow:
        case BackgroundYellow:
            return {0xe5, 0xc0, 0x7b};

        case ForegroundBlue:
        case BackgroundBlue:
            return {0x5c, 0xb0, 0xf7};

        case ForegroundMagenta:
        case BackgroundMagenta:
            return {0xc6, 0x78, 0xdd};

        case ForegroundCyan:
        case BackgroundCyan:
            return {0x56, 0xb6, 0xc2};

        case ForegroundWhite:
        case BackgroundWhite:
            return {0xd7, 0xe3, 0xfe};

        default:
            Panic();
    }
}

Colour::Colour(uint8_t red, uint8_t green, uint8_t blue) : red(red), green(green), blue(blue) {}