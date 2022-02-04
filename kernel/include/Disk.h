#pragma once

#include <stdint.h>

class Disk
{
public:
    virtual uint64_t Read(uint64_t addr, void* buffer, uint64_t count, bool preAllocated) = 0;
    virtual uint64_t Write(uint64_t addr, const void* buffer, uint64_t count) = 0;
    virtual ~Disk() = default;
};