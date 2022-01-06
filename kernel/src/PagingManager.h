#ifndef MISKOS_PAGING_H
#define MISKOS_PAGING_H

#include <stdint.h>
#include "PageFrameAllocator.h"
#include "Memory.h"
#include "Stivale2Interface.h"

enum PagingFlag
{
    Present = 0,
    ReadWrite = 1,
    UserAllowed = 2,
    WriteThrough = 3,
    CacheDisable = 4,
    Accessed = 5,
    NX = 63
};

struct PagingStructureEntry
{
    uint64_t value;
    void SetFlag(PagingFlag flag, bool value);
    bool GetFlag(PagingFlag flag);
    void SetPhysicalAddress(uint64_t physAddr);
    uint64_t GetPhysicalAddress();
    // void SetProtectionKey(uint8_t value);
};

struct PagingStructure
{
    PagingStructureEntry entries[512];
} __attribute__((aligned(0x1000)));

class PagingManager
{
public:
    void InitializePaging();
    void MapMemory(void* virtAddr, void* physAddr);
    void UnmapMemory(void* virtAddr);

private:
    PagingStructure* pml4;
};

#endif
