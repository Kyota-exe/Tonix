#include "Memory/UserspaceAllocator.h"

void* UserspaceAllocator::AllocatePages(uint64_t pageCount)
{
    void* addr = reinterpret_cast<void*>(currentAddr);
    currentAddr += pageCount * 0x1000;
    return addr;
}