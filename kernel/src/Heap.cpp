#include "Memory/PageFrameAllocator.h"
#include "Memory/Memory.h"
#include "Heap.h"
#include "Serial.h"
#include "Math.h"
#include "Assert.h"
#include "Spinlock.h"

uint64_t masterIndex = 0;
uint64_t allocTable[1024];

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

const uint64_t SLABS_COUNT = 10;

struct SlabsPool
{
    Slab slabs[SLABS_COUNT];
};

SlabsPool slabsPool;

void Slab::InitializeSlab(uint64_t _slotSize)
{
    slotSize = _slotSize;
    slabBase = HigherHalf(RequestPageFrame());

    uint64_t slotCount = 0x1000 / slotSize;
    FreeSlot* previous = nullptr;
    for (int slotIndex = (int)(slotCount - 1); slotIndex >= 0; --slotIndex)
    {
        auto freeSlot = (FreeSlot*)(slabBase + slotIndex * slotSize);
        freeSlot->next = previous;
        previous = freeSlot;
    }

    head = previous;
}

static uint64_t allocationsInEffect = 0;
void* Slab::Alloc()
{
    lock.Acquire();

    void* addr = head;

    if (addr == nullptr)
    {
        Serial::Printf("No more %d byte slots left in heap slab.", slotSize);
        Serial::Print("Hanging...");
        while (true) asm("hlt");
    }

//    if (slotSize == 9)
//    {
//        Serial::Printf("ALLOC -------------------------------------------------------> %x", (uint64_t)addr, "");
//        Serial::Printf(" - %d", slotSize);
//        Serial::Print("==============================================");
//        Serial::Printf("Allocations in effect: %d", allocationsInEffect);
//        Serial::Printf("Alloc return addr      %x", (uint64_t)addr);
//        Serial::Printf("New head               %x", (uint64_t)head->next);
//        Serial::Printf("New head->next         %x", (uint64_t)head->next->next);
//        Serial::Printf("nullptr                %x", (uint64_t)nullptr);
//        Serial::Print("==============================================");
//    }

    allocTable[masterIndex++] = (uint64_t)addr;

    head = head->next;
    allocationsInEffect++;

    lock.Release();

    return addr;
}

void Slab::Free(void* ptr)
{
    lock.Acquire();

    FreeSlot* previousHead = head;
    head = (FreeSlot*)ptr;
    head->next = previousHead;

    allocationsInEffect--;

//    if (slotSize == 9)
//    {
//        Serial::Printf("FREE --------------------------------------------------------> %x", (uint64_t)ptr, "");
//        Serial::Printf(" - %d", slotSize);
//        Serial::Print("==============================================");
//        Serial::Printf("Allocations in effect: %d", allocationsInEffect);
//        Serial::Printf("Freed addr:                 %x", (uint64_t)ptr);
//        Serial::Printf("New head:                   %x", (uint64_t)head);
//        Serial::Printf("(prev head) New head->next: %x", (uint64_t)previousHead);
//        Serial::Print("==============================================");
//    }

    bool foundAddr = false;
    for (uint64_t i = 0; i < masterIndex + 1; ++i)
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
    return (void*)((uint64_t)RequestPageFrames(pageCount) + 0xffff'8000'0000'0000);
}

void* KMalloc(uint64_t size)
{
    if (size < 8) size = 8;
    uint64_t slabIndex = CeilLog2(size) - 3;

    if (slabIndex >= SLABS_COUNT)
    {
        return LargeKMalloc(size);
    }

    return slabsPool.slabs[slabIndex].Alloc();
}

void KFree(void* ptr)
{
    if (ptr == nullptr) return;

    for (auto& slab : slabsPool.slabs)
    {
        if (slab.slabBase <= (uint64_t)ptr && (uint64_t)ptr < slab.slabBase + 0x1000)
        {
            slab.Free(ptr);
            return;
        }
    }

    Serial::Printf("Failed to free address %x", ptr);
    Panic();
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

void* operator new(uint64_t, void* ptr) { return ptr; }
void* operator new[](uint64_t, void* ptr) { return ptr; }
void* operator new(uint64_t size) { return KMalloc(size); }
void* operator new[](uint64_t size) { return KMalloc(size); }
void operator delete(void* ptr) { KFree(ptr); }
void operator delete(void* ptr, uint64_t) { KFree(ptr); }
void operator delete[](void* ptr) { KFree(ptr); }
void operator delete[](void* ptr, uint64_t) { KFree(ptr); }
