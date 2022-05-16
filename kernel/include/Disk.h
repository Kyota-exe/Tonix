#pragma once

#include <stdint.h>

class Disk
{
public:
    virtual void Read(uint64_t addr, void* buffer, uint64_t count) = 0;
    virtual void Write(uint64_t addr, const void* buffer, uint64_t count) = 0;
    virtual ~Disk() = default;
    Disk& operator=(const Disk&) = delete;
};
