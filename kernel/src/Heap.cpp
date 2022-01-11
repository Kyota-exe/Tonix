#include "Heap.h"
#include "PageFrameAllocator.h"
#include "Serial.h"

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
};

const uint64_t SLABS_COUNT = 10;

struct SlabsPool
{
    Slab slabs[SLABS_COUNT];
};

SlabsPool* slabsPool;

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
    Serial::Printf("Allocating. Me: %d", slotSize);
    Serial::Printf("Head addr: %x", (uint64_t)head);
    void* addr = head;
    head = head->next;
    return addr;
}

void Slab::Free(void* ptr)
{
    Serial::Printf("%d", slotSize, "");
    Serial::Printf("--------------Freeing: %x", (uint64_t)ptr);
    FreeSlot* previousHead = head;
    head = (FreeSlot*)ptr;
    head->next = previousHead;
    Serial::Printf("NEW HEAD =================> %x", (uint64_t)head);
}

void* KMalloc(uint64_t size)
{
    if (size <= 8) return slabsPool->slabs[0].Alloc();
    if (size <= 16) return slabsPool->slabs[1].Alloc();
    if (size <= 32) return slabsPool->slabs[2].Alloc();
    if (size <= 64) return slabsPool->slabs[3].Alloc();
    if (size <= 128) return slabsPool->slabs[4].Alloc();
    if (size <= 256) return slabsPool->slabs[5].Alloc();
    if (size <= 512) return slabsPool->slabs[6].Alloc();
    if (size <= 1024) return slabsPool->slabs[7].Alloc();
    if (size <= 2048) return slabsPool->slabs[8].Alloc();
    if (size <= 4096) return slabsPool->slabs[9].Alloc();

    Serial::Print("KMalloc failed.");
    Serial::Print("Hanging...");
    while (true) asm("hlt");
}

void KFree(void* ptr)
{
    for (uint64_t slabIndex = 0; slabIndex < SLABS_COUNT; ++slabIndex)
    {
        Slab* slab = &slabsPool->slabs[slabIndex];
        if (slab->slabBase <= (uint64_t)ptr && (uint64_t)ptr < slab->slabBase + 0x1000)
        {
            slab->Free(ptr);
        }
    }
}

void InitializeKernelHeap()
{
    slabsPool = (SlabsPool*)((uint64_t)RequestPageFrame() + 0xffff'8000'0000'0000);
    Serial::Printf("Slabs pool addr: %x", (uint64_t)slabsPool);
    if (sizeof(SlabsPool) > 0x1000)
    {
        Serial::Printf("Total slabs size (%x) is greater than page size.", sizeof(SlabsPool));
    }

    Serial::Printf("Slabs 0: %x", (uint64_t)&(slabsPool->slabs[0]));
    slabsPool->slabs[0].InitializeSlab(8);
    slabsPool->slabs[1].InitializeSlab(16);
    slabsPool->slabs[2].InitializeSlab(32);
    slabsPool->slabs[3].InitializeSlab(64);
    slabsPool->slabs[4].InitializeSlab(128);
    slabsPool->slabs[5].InitializeSlab(256);
    slabsPool->slabs[6].InitializeSlab(512);
    slabsPool->slabs[7].InitializeSlab(1024);
    slabsPool->slabs[8].InitializeSlab(2048);
    slabsPool->slabs[9].InitializeSlab(4096);
}