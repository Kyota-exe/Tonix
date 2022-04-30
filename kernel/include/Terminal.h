#pragma once

#include "Device.h"
#include "PSF2.h"
#include "TextRenderer.h"
#include "Vector.h"

class Terminal
{
public:
    void Write(const String& string);
    Terminal();
    ~Terminal();
    Terminal& operator=(const Terminal&) = delete;

private:
    struct EscapeSequence;

    void ProcessEscapeSequence(const EscapeSequence& escapeSequence);
    void ProcessControlSequence(char command, const Vector<unsigned int>& arguments, bool decPrivate);
    bool CursorAtRightEdge();
    void ResetColors();
    void EraseRangeInclusive(long minX, long minY, long maxX, long maxY);
    void EraseScreenFrom(long x, long y);

    TextRenderer* textRenderer;
    Vector<uint64_t> unblockQueue;

    Colour backgroundColour;
    Colour originalTextColour;
    Colour textColour;
    Colour textBgColour;

    long cursorX;
    long cursorY;

    bool pendingWrap;
};

struct Terminal::EscapeSequence
{
    char command;
    bool controlSequence;
    bool decPrivate;
    bool rightParentheses;
    Vector<unsigned int> controlArguments;
};