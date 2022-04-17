#pragma once

#include "Device.h"
#include "PSF2Font.h"
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
    void Print(const String& string);
    void ProcessEscapeSequence(char command, bool hasCSI, const Vector<unsigned int>& arguments);

    TextRenderer* textRenderer;
    Vector<uint64_t> unblockQueue;

    Colour backgroundColour;
    Colour textColour;
    Colour textBgColour;

    long cursorX;
    long cursorY;

    char* inputBuffer;
    uint64_t currentBufferLength;
};