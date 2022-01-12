#include "Heap.h"
#include "PageFrameAllocator.h"
#include "Serial.h"
#include "Math.h"

struct FreeSlot
{
    FreeSlot* next;
};

class Slab
{
private:
    uint64_t slotSize;
    FreeSlot* head;
    Slab(const Slab& original);
public:
    uint64_t slabBase;
    void InitializeSlab(uint64_t _slotSize);
    void* Alloc();
    void Free(void* ptr);
    Slab();
};

const uint64_t SLABS_COUNT = 10;

struct SlabsPool
{
    Slab slabs[SLABS_COUNT];
};

SlabsPool slabsPool;

Slab::Slab() { }

void Slab::InitializeSlab(uint64_t _slotSize)
{
    slotSize = _slotSize;
    slabBase = (uint64_t)RequestPageFrame() + 0xffff'8000'0000'0000;

    uint64_t slotCount = 0x1000 / slotSize;
    FreeSlot* previous = 0;
    for (int slotIndex = slotCount - 1; slotIndex >= 0; --slotIndex)
    {
        FreeSlot* freeSlot = (FreeSlot*)(slabBase + slotIndex * slotSize);
        freeSlot->next = previous;
        previous = freeSlot;
    }

    head = previous;
}

void* Slab::Alloc()
{
    void* addr = head;

    if (addr == 0)
    {
        Serial::Printf("No more %d byte slots left in heap slab.", slotSize);
        Serial::Print("Hanging...");
        while (true) asm("hlt");
    }

    head = head->next;
    return addr;
}

void Slab::Free(void* ptr)
{
    FreeSlot* previousHead = head;
    head = (FreeSlot*)ptr;
    head->next = previousHead;
}

void* KMalloc(uint64_t size)
{
    if (size < 8) size = 8;
    uint64_t slabIndex = CeilLog2(size) - 3;
    if (slabIndex >= SLABS_COUNT)
    {
        Serial::Printf("KMalloc does not support allocations of size %d.", size);
        Serial::Print("Hanging...");
        while (true) asm("hlt");
    }
    return slabsPool.slabs[slabIndex].Alloc();
}

void KFree(void* ptr)
{
    for (uint64_t slabIndex = 0; slabIndex < SLABS_COUNT; ++slabIndex)
    {
        Slab* slab = &slabsPool.slabs[slabIndex];
        if (slab->slabBase <= (uint64_t)ptr && (uint64_t)ptr < slab->slabBase + 0x1000)
        {
            slab->Free(ptr);
        }
    }
}

void InitializeKernelHeap()
{
    slabsPool.slabs[0].InitializeSlab(8);
    slabsPool.slabs[1].InitializeSlab(16);
    slabsPool.slabs[2].InitializeSlab(32);
    slabsPool.slabs[3].InitializeSlab(64);
    slabsPool.slabs[4].InitializeSlab(128);
    slabsPool.slabs[5].InitializeSlab(256);
    slabsPool.slabs[6].InitializeSlab(512);
    slabsPool.slabs[7].InitializeSlab(1024);
    slabsPool.slabs[8].InitializeSlab(2048);
    slabsPool.slabs[9].InitializeSlab(4096);
}