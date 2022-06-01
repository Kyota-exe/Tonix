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
    uint64_t slabSize;
    void InitializeSlab(uint64_t _slotSize, uint64_t _slabSize);
    void* Alloc();
    void Free(void* ptr);

    Slab() = default;
    Slab(const Slab& original) = delete;
};

constexpr uint64_t SLABS_COUNT = 10;
Slab slabs[SLABS_COUNT];

uint64_t allocTableIndex = 0;
constexpr uint64_t ALLOC_TABLE_SIZE = 8192;
uint64_t allocTable[ALLOC_TABLE_SIZE];
constexpr bool DEBUG_DOUBLE_FREE = false;

void Slab::InitializeSlab(uint64_t _slotSize, uint64_t _slabSize)
{
    Assert(slabSize % 0x1000 == 0);
    slabSize = _slabSize;
    slotSize = _slotSize;

    slabBase = HigherHalf(RequestPageFrames(slabSize / 0x1000));

    FreeSlot* previous = nullptr;
    for (uint64_t slotIndex = slabSize / slotSize; slotIndex-- > 0; )
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

    if (DEBUG_DOUBLE_FREE)
    {
        Assert(allocTableIndex < ALLOC_TABLE_SIZE);
        allocTable[allocTableIndex++] = reinterpret_cast<uintptr_t>(addr);
    }

    lock.Release();
    return addr;
}

void Slab::Free(void* ptr)
{
    lock.Acquire();

    FreeSlot* previousHead = head;
    head = static_cast<FreeSlot*>(ptr);
    head->next = previousHead;

    if (DEBUG_DOUBLE_FREE)
    {
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
            Serial::Log("Double free!");
            Serial::Log("DF addr: %x", (uint64_t) ptr);
            Serial::Log("DF Slot size: %d", (uint64_t) slotSize);
            Panic();
        }
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
    Assert(slabIndex < SLABS_COUNT);
    return slabs[slabIndex].Alloc();
}

void KFree(void* ptr)
{
    Assert(ptr != nullptr);

    auto address = reinterpret_cast<uintptr_t>(ptr);
    for (auto& slab : slabs)
    {
        if (address >= slab.slabBase && address < slab.slabBase + slab.slabSize)
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
        slabs[i].InitializeSlab(Pow(2, 3 + i), 0x1000 * 2);
    }
}

Spinlock permanentAllocatorLock;
uintptr_t currentPageAddr;
uint64_t currentOffset = 0x1000;
void* PermanentAlloc(uint64_t size)
{
    void* ptr;

    if (size > 0x1000)
    {
        ptr = LargeKMalloc(size);
    }
    else if (currentOffset + size <= 0x1000)
    {
        permanentAllocatorLock.Acquire();
        ptr = reinterpret_cast<void*>(currentPageAddr + currentOffset);
        currentOffset += size;
        permanentAllocatorLock.Release();
    }
    else
    {
        permanentAllocatorLock.Acquire();
        currentPageAddr = HigherHalf(RequestPageFrame());
        currentOffset = size;
        permanentAllocatorLock.Release();

        ptr = reinterpret_cast<void*>(currentPageAddr);
    }

    Assert(ptr != nullptr);
    return ptr;
}

void* operator new(uint64_t, void* ptr) { return ptr; }
void* operator new[](uint64_t, void* ptr) { return ptr; }
void* operator new(uint64_t size) { return KMalloc(size); }
void* operator new[](uint64_t size) { return KMalloc(size); }
void operator delete(void* ptr) { KFree(ptr); }
void operator delete(void* ptr, uint64_t) { KFree(ptr); }
void operator delete[](void* ptr) { KFree(ptr); }
void operator delete[](void* ptr, uint64_t) { KFree(ptr); }

void* operator new(uint64_t size, Allocator type)
{
    switch (type)
    {
        case Allocator::Permanent: return PermanentAlloc(size);
        case Allocator::Slab: return KMalloc(size);
        default: Panic();
    }
}

void* operator new[](uint64_t size, Allocator type)
{
    switch (type)
    {
        case Allocator::Permanent: return PermanentAlloc(size);
        case Allocator::Slab: return KMalloc(size);
        default: Panic();
    }
}