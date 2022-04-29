#pragma once

#include "Device.h"
#include "PSF2.h"
#include "TextRenderer.h"
#include "Vector.h"

class Terminal : public Device
{
public:
    uint64_t Read(void* buffer, uint64_t count) override;
    uint64_t Write(const void* buffer, uint64_t count) override;
    Terminal(const String& name, uint32_t inodeNum);
    ~Terminal();
    Terminal& operator=(const Terminal&) = delete;

    void InputCharacter(char c);
    static Terminal* instance;

private:
    struct EscapeSequence;

    void Print(const String& string);
    void ProcessEscapeSequence(const EscapeSequence& escapeSequence);
    void ProcessControlSequence(char command, const Vector<unsigned int>& arguments, bool decPrivate);
    void ResetColors();
    void EraseRangeInclusive(long minX, long minY, long maxX, long maxY);
    void EraseScreenFrom(long x, long y);

    TextRenderer* textRenderer;
    Vector<uint64_t> unblockQueue;

    Colour backgroundColour;

    Colour originalTextColour;
    Colour originalTextBgColour;

    Colour textColour;
    Colour textBgColour;

    long cursorX;
    long cursorY;

    char* inputBuffer;
    uint64_t currentBufferLength;

    Vector<long> nextCursorXPerLine;
};

struct Terminal::EscapeSequence
{
    char command;
    bool controlSequence;
    bool decPrivate;
    bool rightParentheses;
    Vector<unsigned int> controlArguments;
};