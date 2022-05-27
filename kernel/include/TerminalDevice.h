#pragma once

#include <stdint.h>
#include "Device.h"
#include "Vector.h"
#include "Terminal.h"

class TerminalDevice : public Device
{
public:
    uint64_t Read(void* buffer, uint64_t count) override;
    uint64_t Write(const void* buffer, uint64_t count) override;
    void KeyboardInput(char c);
    TerminalDevice(const String& name, uint32_t inodeNum);
    WindowSize GetWindowSize();
    static TerminalDevice* instance;
    bool canonical;
    bool echo;
private:
    struct ReadRequest;
    Vector<Vector<char>> lines;
    Vector<char> rawBuffer;
    Vector<ReadRequest> unblockQueue;
    Terminal* terminal;
};

struct TerminalDevice::ReadRequest
{
    uint64_t pid;
    uint64_t requestedCount;
};