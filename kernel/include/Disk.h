#pragma once

#include <stdint.h>

class Disk
{
public:
    virtual void Read(uint64_t addr, void* buffer, uint64_t count) = 0;
    virtual void Write(uint64_t addr, const void* buffer, uint64_t count) = 0;
    virtual void AllocateRead(uint64_t addr, void** bufferPtr, uint64_t count);
    virtual ~Disk() = default;
    Disk& operator=(const Disk&) = delete;
};
