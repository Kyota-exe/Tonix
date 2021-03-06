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
    void CopyUserspace(PagingManager& original);
    void SetCR3() const;
    void MapMemory(const void* virtAddr, const void* physAddr);
    unsigned int FlagMismatchLevel(const void* virtAddr, PagingFlag flag, bool enabled);
    unsigned int PageNotPresentLevel(const void* virtAddr);
    static void SaveBootloaderAddressSpace();
    uintptr_t pml4PhysAddr {};
private:
    struct PageTableEntry;
    PageTableEntry* pml4 {};
    Spinlock lock;
    static PageTableEntry* defaultPml4;
    static void GetPageTableIndexes(const void* virtAddr, uint16_t* pageIndexes);
    static void PopulatePagingStructureEntry(PageTableEntry& entry, uintptr_t physAddr);
    static PageTableEntry* AllocatePagingStructure(PageTableEntry& entry);
    void CopyPages(const PageTableEntry* originalTable, PageTableEntry* table, uint64_t pageCount, unsigned int level);
};

struct PagingManager::PageTableEntry
{
    uint64_t value;
    void SetFlag(PagingFlag flag, bool value);
    bool GetFlag(PagingFlag flag) const;
    void SetPhysicalAddress(uint64_t physAddr);
    uint64_t GetPhysicalAddress() const;
} __attribute__((packed));