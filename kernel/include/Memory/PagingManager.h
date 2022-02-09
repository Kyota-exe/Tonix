#pragma once

#include <stdint.h>

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
    bool GetFlag(PagingFlag flag) const;
    void SetPhysicalAddress(uint64_t physAddr);
    uint64_t GetPhysicalAddress() const;
};

struct PagingStructure
{
    PagingStructureEntry entries[512];
} __attribute__((aligned(0x1000)));

class PagingManager
{
public:
    void InitializePaging();
    void SetCR3() const;
    void MapMemory(const void* virtAddr, const void* physAddr, bool user);
    void UnmapMemory(const void* virtAddr);
    uint8_t PageNotPresentLevel(const void* virtAddr);
    uintptr_t pml4PhysAddr;
private:
    PagingStructure* pml4;
	static void GetPageTableIndexes(const void* virtAddr, uint16_t& pageIndex, uint16_t& pageTableIndex,
									uint16_t& pageDirectoryIndex, uint16_t& pdptIndex);
	static void PopulatePagingStructureEntry(PagingStructureEntry* entry, uintptr_t physAddr, bool user);
	static PagingStructure* AllocatePagingStructure(PagingStructureEntry* entry, bool user);
};