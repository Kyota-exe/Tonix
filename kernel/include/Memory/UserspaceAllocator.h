#pragma once

#include <stdint.h>

class UserspaceAllocator
{
public:
    void* AllocatePages(uint64_t pageCount);
private:
    uintptr_t currentAddr {0x1'0000'0000};
};