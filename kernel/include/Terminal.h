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
    void InputCharacter(char c);
    Terminal(const String& name, uint32_t inodeNum);
    ~Terminal();
    Terminal& operator=(const Terminal&) = delete;
    static Terminal* instance;
private:
    TextRenderer* textRenderer;
    char* inputBuffer;
    uint64_t currentBufferLength;
    Vector<uint64_t> unblockQueue;
};