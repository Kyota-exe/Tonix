#pragma once

#include "Device.h"
#include "PSF2Font.h"
#include "TextRenderer.h"

class Terminal : public Device
{
public:
    uint64_t Read(void* buffer, uint64_t count) override;
    uint64_t Write(const void* buffer, uint64_t count) override;
    Terminal(const String& name, uint32_t inodeNum);
    ~Terminal();
    TextRenderer* textRenderer;
    Colour textColour;
private:
    unsigned int cursorX = 0;
    unsigned int cursorY = 0;
};