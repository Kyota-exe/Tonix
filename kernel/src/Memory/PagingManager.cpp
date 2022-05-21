#include "Memory/Memory.h"
#include "Memory/PageFrameAllocator.h"
#include "Memory/PagingManager.h"
#include "Stivale2Interface.h"
#include "Assert.h"

constexpr unsigned int PAGING_LEVELS = 4;
PagingManager::PageTableEntry* PagingManager::defaultPml4 = nullptr;

void PagingManager::GetPageTableIndexes(const void* virtAddr, uint16_t* pageIndexes)
{
    auto virtualAddress = reinterpret_cast<uintptr_t>(virtAddr);

    virtualAddress >>= 12;
    for (unsigned int level = 0; level < PAGING_LEVELS; ++level)
    {
        pageIndexes[level] = virtualAddress & 0b111'111'111;
        virtualAddress >>= 9;
    }
}

void PagingManager::PopulatePagingStructureEntry(PageTableEntry& entry, uintptr_t physAddr)
{
    entry.SetPhysicalAddress(physAddr);

    // Flags
    entry.SetFlag(PagingFlag::Present, true);
    entry.SetFlag(PagingFlag::AllowWrite, true);
    entry.SetFlag(PagingFlag::UserAllowed, true);
}

PagingManager::PageTableEntry* PagingManager::AllocatePagingStructure(PageTableEntry& entry)
{
    auto physAddr = RequestPageFrame();

    auto pagingStruct = reinterpret_cast<PageTableEntry*>(HigherHalf(physAddr));
    memset(pagingStruct, 0, 0x1000);

    PopulatePagingStructureEntry(entry, physAddr);

    return pagingStruct;
}

void PagingManager::InitializePaging()
{
    Assert(pml4 == nullptr);
    lock.Acquire();

    pml4PhysAddr = RequestPageFrame();
    pml4 = reinterpret_cast<PageTableEntry*>(HigherHalf(pml4PhysAddr));
    memset(pml4, 0, 0x1000 / 2);

    Assert(defaultPml4 != nullptr);
    memcpy(pml4 + 256, defaultPml4 + 256, 0x1000 / 2);

    lock.Release();
}

void PagingManager::CopyUserspace(PagingManager& original)
{
    original.lock.Acquire();
    lock.Acquire();
    CopyPages(original.pml4, pml4, 256, PAGING_LEVELS - 1);
    lock.Release();
    original.lock.Release();
}

void PagingManager::CopyPages(const PageTableEntry* originalTable, PageTableEntry* table, uint64_t pageCount, unsigned int level)
{
    for (uint64_t entryIndex = 0; entryIndex < pageCount; ++entryIndex)
    {
        const auto& originalEntry = originalTable[entryIndex];
        if (originalEntry.GetFlag(PagingFlag::Present))
        {
            auto& entry = table[entryIndex];
            Assert(!entry.GetFlag(PagingFlag::Present));

            memcpy(&entry, &originalEntry, sizeof(PageTableEntry));
            auto originalNext = reinterpret_cast<PageTableEntry*>(HigherHalf(entry.GetPhysicalAddress()));

            uintptr_t nextPhysAddr = RequestPageFrame();
            entry.SetPhysicalAddress(nextPhysAddr);
            auto next = reinterpret_cast<void*>(HigherHalf(nextPhysAddr));

            if (level > 0)
            {
                memset(next, 0, 0x1000);
                CopyPages(static_cast<const PageTableEntry*>(originalNext), static_cast<PageTableEntry*>(next), 512, level - 1);
            }
            else
            {
                memcpy(next, originalNext, 0x1000);
            }
        }
    }
}

void PagingManager::SetCR3() const
{
    asm volatile("mov %0, %%cr3" : : "r" (pml4PhysAddr));
}

void PagingManager::MapMemory(const void* virtAddr, const void* physAddr)
{
    uint16_t pageIndexes[PAGING_LEVELS];
    GetPageTableIndexes(virtAddr, pageIndexes);

    auto table = pml4;
    lock.Acquire();
    for (unsigned int level = PAGING_LEVELS - 1; level > 0; --level)
    {
        auto& entry = table[pageIndexes[level]];
        if (entry.GetFlag(PagingFlag::Present))
        {
            table = reinterpret_cast<PageTableEntry*>(HigherHalf(entry.GetPhysicalAddress()));
        }
        else
        {
            table = AllocatePagingStructure(entry);
        }
    }

    PageTableEntry& page = table[pageIndexes[0]];
    Assert(!page.GetFlag(PagingFlag::Present));

    PopulatePagingStructureEntry(page, reinterpret_cast<uintptr_t>(physAddr));
    lock.Release();
}

unsigned int PagingManager::FlagMismatchLevel(const void* virtAddr, PagingFlag flag, bool enabled)
{
    uint16_t pageIndexes[PAGING_LEVELS];
    GetPageTableIndexes(virtAddr, pageIndexes);

    lock.Acquire();
    auto table = pml4;
    for (unsigned int level = PAGING_LEVELS - 1; level > 0; --level)
    {
        auto i = pageIndexes[level];
        auto& entry = table[i];
        if (entry.GetFlag(flag) != enabled)
        {
            return level + 1;
        }
        table = reinterpret_cast<PageTableEntry*>(HigherHalf(entry.GetPhysicalAddress()));
    }

    bool result = table[pageIndexes[0]].GetFlag(flag) != enabled;
    lock.Release();
    return result;
}

unsigned int PagingManager::PageNotPresentLevel(const void* virtAddr)
{
    return FlagMismatchLevel(virtAddr, PagingFlag::Present, true);
}

void PagingManager::SaveBootloaderAddressSpace()
{
    Assert(defaultPml4 == nullptr);
    uint64_t bootloaderPml4PhysAddr;
    asm volatile("mov %%cr3, %0" : "=r"(bootloaderPml4PhysAddr));
    defaultPml4 = reinterpret_cast<PageTableEntry*>(HigherHalf(bootloaderPml4PhysAddr));
}

void PagingManager::PageTableEntry::SetFlag(PagingFlag flag, bool enable)
{
    if (enable) value |= (1 << (uint64_t)flag);
    else value &= ~(1 << (uint64_t)flag);
}

bool PagingManager::PageTableEntry::GetFlag(PagingFlag flag) const
{
    return value & (1 << (uint64_t)flag);
}

void PagingManager::PageTableEntry::SetPhysicalAddress(uint64_t physAddr)
{
    physAddr &= 0x0000'fffffffff'000;
    value &=    0xffff'000000000'fff;
    value |= physAddr;
}

uint64_t PagingManager::PageTableEntry::GetPhysicalAddress() const
{
    return value & 0x0000'fffffffff'000;
}