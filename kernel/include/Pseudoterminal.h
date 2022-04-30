#pragma once

#include <stdint.h>
#include "Device.h"
#include "Vector.h"
#include "Terminal.h"

class Pseudoterminal : public Device
{
public:
    uint64_t Read(void* buffer, uint64_t count) override;
    uint64_t Write(const void* buffer, uint64_t count) override;
    void KeyboardInput(char c);
    Pseudoterminal(const String& name, uint32_t inodeNum);
    static Pseudoterminal* instance;
private:
    struct ReadRequest;
    Vector<Vector<char>> lines;
    Vector<ReadRequest> unblockQueue;
    Terminal* terminal;
};

struct Pseudoterminal::ReadRequest
{
    uint64_t pid;
    uint64_t requestedCount;
};