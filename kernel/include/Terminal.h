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
    void ProcessControlSequence(char command, const Vector<int>& arguments, bool decPrivate);
    bool CursorAtRightEdge();
    void ResetColors();
    void EraseRangeInclusive(long minX, long minY, long maxX, long maxY);
    void EraseScreenFrom(long x, long y);

    TextRenderer* textRenderer;
    Vector<uint64_t> unblockQueue;

    Colour textColour = Colour(255, 255, 255);
    const Colour backgroundColour = Colour(0, 0, 0);

    const Colour originalTextColour;
    Colour textBgColour;

    long cursorX = 0;
    long cursorY = 0;

    bool pendingWrap = false;
};

struct Terminal::EscapeSequence
{
    char command;
    bool controlSequence;
    bool decPrivate;
    bool rightParentheses;
    Vector<int> controlArguments;
};