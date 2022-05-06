#pragma once

#include <stdint.h>
#include "Spinlock.h"

enum class PagingFlag : uint64_t
{
    Present = 0,
    AllowWrite = 1,
    UserAllowed = 2,
    WriteThrough = 3,
    CacheDisable = 4,
    Accessed = 5,
    NX = 63
};

class PagingManager
{
public:
    void InitializePaging();
    void SetCR3() const;
    void MapMemory(const void* virtAddr, const void* physAddr, bool user);
    void UnmapMemory(const void* virtAddr);
    unsigned int FlagMismatchLevel(const void* virtAddr, PagingFlag flag, bool enabled);
    unsigned int PageNotPresentLevel(const void* virtAddr);
    bool AddressIsAccessible(const void* virtAddr);
    uintptr_t GetPageTableEntryVirtAddr(const void* virtAddr);
    uintptr_t GetTranslatedPhysAddr(const void* virtAddr);
    uintptr_t pml4PhysAddr {};
private:
    struct PageTableEntry;
    struct PageTable;
    PageTable* pml4 {};
    Spinlock lock;
	static void GetPageTableIndexes(const void* virtAddr, uint16_t* pageIndexes);
	static void PopulatePagingStructureEntry(PageTableEntry& entry, uintptr_t physAddr, bool user);
	static PageTable* AllocatePagingStructure(PageTableEntry& entry, bool user);
};

struct PagingManager::PageTableEntry
{
    uint64_t value;
    void SetFlag(PagingFlag flag, bool value);
    bool GetFlag(PagingFlag flag) const;
    void SetPhysicalAddress(uint64_t physAddr);
    uint64_t GetPhysicalAddress() const;
} __attribute__((packed));

struct PagingManager::PageTable
{
    PageTableEntry entries[512];
} __attribute__((packed, aligned(0x1000)));