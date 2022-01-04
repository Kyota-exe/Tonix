#include "Heap.h"

static uint64_t currentSlabAddr = 0;
static uint64_t currentSlabOffset = 0;

void* KMalloc(uint64_t size)
{
    if (currentSlabOffset + size > 0x1000 || currentSlabAddr == 0)
    {
        if (size > 0x1000)
        {
            Serial::Print("Kernel heap cannot allocate more than 0x1000 bytes at a time.");
            Serial::Print("Hanging...");
            while (true) asm("hlt");
        }
        currentSlabAddr = (uint64_t)PageFrameAllocator::RequestPageFrame() + 0xffff'8000'0000'0000;
    }
    void* addr = (void*)(currentSlabAddr + currentSlabOffset);
    currentSlabOffset += size;
    return addr;
}

void KFree(void* ptr)
{
    (void)ptr;
    // Not implemented
}