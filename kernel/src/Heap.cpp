#include "Memory/PageFrameAllocator.h"
#include "Memory/Memory.h"
#include "Heap.h"
#include "Serial.h"
#include "Math.h"
#include "Assert.h"
#include "Spinlock.h"

struct FreeSlot
{
    FreeSlot* next;
};

class Slab
{
private:
    uint64_t slotSize;
    FreeSlot* head;
    Spinlock lock;

public:
    uint64_t slabBase;
    void InitializeSlab(uint64_t _slotSize);
    void* Alloc();
    void Free(void* ptr);

    Slab() = default;
    Slab(const Slab& original) = delete;
};

constexpr uint64_t SLABS_COUNT = 10;
Slab slabs[SLABS_COUNT];

uint64_t allocTableIndex = 0;
constexpr uint64_t ALLOC_TABLE_SIZE = 4096;
uint64_t allocTable[ALLOC_TABLE_SIZE];

void Slab::InitializeSlab(uint64_t _slotSize)
{
    slotSize = _slotSize;
    slabBase = HigherHalf(RequestPageFrame());

    uint64_t slotCount = 0x1000 / slotSize;
    FreeSlot* previous = nullptr;
    for (uint64_t slotIndex = slotCount; slotIndex-- > 0; )
    {
        auto freeSlot = reinterpret_cast<FreeSlot*>(slabBase + slotIndex * slotSize);
        freeSlot->next = previous;
        previous = freeSlot;
    }

    head = previous;
}

void* Slab::Alloc()
{
    lock.Acquire();

    void* addr = head;
    Assert(addr != nullptr);
    head = head->next;

    Assert(allocTableIndex < ALLOC_TABLE_SIZE);
    allocTable[allocTableIndex++] = reinterpret_cast<uintptr_t>(addr);

    lock.Release();

    return addr;
}

void Slab::Free(void* ptr)
{
    lock.Acquire();

    FreeSlot* previousHead = head;
    head = reinterpret_cast<FreeSlot*>(ptr);
    head->next = previousHead;

    bool foundAddr = false;
    for (uint64_t i = 0; i < allocTableIndex + 1; ++i)
    {
        if (allocTable[i] == (uint64_t)ptr)
        {
            allocTable[i] = 0;
            foundAddr = true;
            break;
        }
    }
    if (!foundAddr)
    {
        Serial::Print("Double free!");
        Serial::Printf("DF addr: %x", (uint64_t)ptr);
        Serial::Printf("DF Slot size: %d", (uint64_t)slotSize);
        Panic();
    }

    lock.Release();
}

void* LargeKMalloc(uint64_t size)
{
    uint64_t pageCount = size / 0x1000;
    return reinterpret_cast<void*>(HigherHalf(RequestPageFrames(pageCount)));
}

void* KMalloc(uint64_t size)
{
    if (size < 8) size = 8;
    uint64_t slabIndex = CeilLog2(size) - 3;

    if (slabIndex >= SLABS_COUNT)
    {
        return LargeKMalloc(size);
    }

    return slabs[slabIndex].Alloc();
}

void KFree(void* ptr)
{
    Assert(ptr != nullptr);

    auto address = reinterpret_cast<uintptr_t>(ptr);
    for (auto& slab : slabs)
    {
        if (slab.slabBase <= address && address < slab.slabBase + 0x1000)
        {
            slab.Free(ptr);
            return;
        }
    }

    Panic();
}

void InitializeKernelHeap()
{
    for (uint64_t i = 0; i < SLABS_COUNT; ++i)
    {
        slabs[i].InitializeSlab(Pow(2, 3 + i));
    }
}

void* operator new(uint64_t, void* ptr) { return ptr; }
void* operator new[](uint64_t, void* ptr) { return ptr; }
void* operator new(uint64_t size) { return KMalloc(size); }
void* operator new[](uint64_t size) { return KMalloc(size); }
void operator delete(void* ptr) { KFree(ptr); }
void operator delete(void* ptr, uint64_t) { KFree(ptr); }
void operator delete[](void* ptr) { KFree(ptr); }
void operator delete[](void* ptr, uint64_t) { KFree(ptr); }
