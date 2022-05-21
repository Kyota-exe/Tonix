#include "Memory/Memory.h"
#include "Memory/PageFrameAllocator.h"
#include "Memory/PagingManager.h"
#include "Stivale2Interface.h"
#include "Assert.h"

constexpr unsigned int PAGING_LEVELS = 4;

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

PagingManager::PageTable* PagingManager::AllocatePagingStructure(PageTableEntry& entry)
{
    auto physAddr = RequestPageFrame();

    auto pagingStruct = reinterpret_cast<PageTable*>(HigherHalf(physAddr));
    memset(pagingStruct, 0, 0x1000);

    PopulatePagingStructureEntry(entry, physAddr);

    return pagingStruct;
}

void PagingManager::InitializePaging()
{
    lock.Acquire();
    Assert(pml4 == nullptr);
    pml4PhysAddr = RequestPageFrame();
    pml4 = reinterpret_cast<PageTable*>(HigherHalf(pml4PhysAddr));
    memset(pml4, 0, 0x1000);
    lock.Release();

    for (uint64_t pageFrameIndex = 0; pageFrameIndex < pageFrameCount; ++pageFrameIndex)
    {
        uintptr_t physAddr = pageFrameIndex * 0x1000;
        MapMemory(reinterpret_cast<void*>(HigherHalf(physAddr)), reinterpret_cast<void*>(physAddr));
    }

    auto kernelBaseAddrStruct = reinterpret_cast<stivale2_struct_tag_kernel_base_address*>(GetStivale2Tag(STIVALE2_STRUCT_TAG_KERNEL_BASE_ADDRESS_ID));
    auto pmrsStruct = reinterpret_cast<stivale2_struct_tag_pmrs*>(GetStivale2Tag(STIVALE2_STRUCT_TAG_PMRS_ID));
    for (uint64_t pmrIndex = 0; pmrIndex < pmrsStruct->entries; ++pmrIndex)
    {
        stivale2_pmr& pmr = pmrsStruct->pmrs[pmrIndex];
        for (uint64_t pmrVirtAddr = pmr.base; pmrVirtAddr < pmr.base + pmr.length; pmrVirtAddr += 0x1000)
        {
            uint64_t physAddr = kernelBaseAddrStruct->physical_base_address + (pmrVirtAddr - kernelBaseAddrStruct->virtual_base_address);
            MapMemory(reinterpret_cast<void*>(pmrVirtAddr), reinterpret_cast<void*>(physAddr));
        }
    }
}

void PagingManager::CopyUserspace(PagingManager& original)
{
    original.lock.Acquire();
    lock.Acquire();
    CopyPages(original.pml4, pml4, 256, PAGING_LEVELS - 1);
    lock.Release();
    original.lock.Release();
}

void PagingManager::CopyPages(const PageTable* originalTable, PageTable* table, uint64_t pageCount, unsigned int level)
{
    for (uint64_t entryIndex = 0; entryIndex < pageCount; ++entryIndex)
    {
        const auto& originalEntry = originalTable->entries[entryIndex];
        if (originalEntry.GetFlag(PagingFlag::Present))
        {
            auto& entry = table->entries[entryIndex];
            Assert(!entry.GetFlag(PagingFlag::Present));

            memcpy(&entry, &originalEntry, sizeof(PageTableEntry));
            auto originalNext = reinterpret_cast<PageTable*>(HigherHalf(entry.GetPhysicalAddress()));

            uintptr_t nextPhysAddr = RequestPageFrame();
            entry.SetPhysicalAddress(nextPhysAddr);
            auto next = reinterpret_cast<void*>(HigherHalf(nextPhysAddr));

            if (level > 0)
            {
                memset(next, 0, 0x1000);
                CopyPages(static_cast<const PageTable*>(originalNext), static_cast<PageTable*>(next), 512, level - 1);
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

    PageTable* table = pml4;
    lock.Acquire();
    for (unsigned int level = PAGING_LEVELS - 1; level > 0; --level)
    {
        auto& entry = table->entries[pageIndexes[level]];
        if (entry.GetFlag(PagingFlag::Present))
        {
            table = reinterpret_cast<PageTable*>(HigherHalf(entry.GetPhysicalAddress()));
        }
        else
        {
            table = AllocatePagingStructure(entry);
        }
    }

    PageTableEntry& page = table->entries[pageIndexes[0]];
    Assert(!page.GetFlag(PagingFlag::Present));

    PopulatePagingStructureEntry(page, reinterpret_cast<uintptr_t>(physAddr));
    lock.Release();
}

void PagingManager::UnmapMemory(const void* virtAddr)
{
    uint16_t pageIndexes[PAGING_LEVELS];
    GetPageTableIndexes(virtAddr, pageIndexes);

    PageTable* table = pml4;
    lock.Acquire();
    for (unsigned int level = PAGING_LEVELS - 1; level > 0; --level)
    {
        auto i = pageIndexes[level];
        PageTableEntry& entry = table->entries[i];
        if (!entry.GetFlag(PagingFlag::Present)) return;
        table = reinterpret_cast<PageTable*>(HigherHalf(entry.GetPhysicalAddress()));
    }

    table->entries[pageIndexes[0]].SetFlag(PagingFlag::Present, false);
    asm volatile("invlpg (%0)" : : "b"(virtAddr) : "memory");
    lock.Release();
}

unsigned int PagingManager::FlagMismatchLevel(const void* virtAddr, PagingFlag flag, bool enabled)
{
    uint16_t pageIndexes[PAGING_LEVELS];
    GetPageTableIndexes(virtAddr, pageIndexes);

    lock.Acquire();
    PageTable* table = pml4;
    for (unsigned int level = PAGING_LEVELS - 1; level > 0; --level)
    {
        auto i = pageIndexes[level];
        auto& entry = table->entries[i];
        if (entry.GetFlag(flag) != enabled)
        {
            return level + 1;
        }
        table = reinterpret_cast<PageTable*>(HigherHalf(entry.GetPhysicalAddress()));
    }

    bool result = table->entries[pageIndexes[0]].GetFlag(flag) != enabled;
    lock.Release();
    return result;
}

unsigned int PagingManager::PageNotPresentLevel(const void* virtAddr)
{
    return FlagMismatchLevel(virtAddr, PagingFlag::Present, true);
}

uintptr_t PagingManager::GetPageTableEntryVirtAddr(const void* virtAddr)
{
    uint16_t pageIndexes[PAGING_LEVELS];
    GetPageTableIndexes(virtAddr, pageIndexes);

    lock.Acquire();
    PageTable* table = pml4;
    for (unsigned int level = PAGING_LEVELS - 1; level > 0; --level)
    {
        PageTableEntry& entry = table->entries[pageIndexes[level]];
        if (!entry.GetFlag(PagingFlag::Present)) return 0;
        table = reinterpret_cast<PageTable*>(HigherHalf(entry.GetPhysicalAddress()));
    }

    auto result = reinterpret_cast<uintptr_t>(&table->entries[pageIndexes[0]]);
    lock.Release();
    return result;
}

uintptr_t PagingManager::GetTranslatedPhysAddr(const void* virtAddr)
{
    uint16_t pageIndexes[PAGING_LEVELS];
    GetPageTableIndexes(virtAddr, pageIndexes);

    PageTable* table = pml4;
    lock.Acquire();
    for (unsigned int level = PAGING_LEVELS - 1; level > 0; --level)
    {
        PageTableEntry& entry = table->entries[pageIndexes[level]];
        if (!entry.GetFlag(PagingFlag::Present)) return 0;
        table = reinterpret_cast<PageTable*>(HigherHalf(entry.GetPhysicalAddress()));
    }

    uintptr_t physAddr = table->entries[pageIndexes[0]].GetPhysicalAddress();
    lock.Release();
    return physAddr;
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