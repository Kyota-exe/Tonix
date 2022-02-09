#include "Memory/Memory.h"
#include "Memory/PageFrameAllocator.h"
#include "Memory/PagingManager.h"
#include "Stivale2Interface.h"
#include "stddef.h"

void PagingManager::GetPageTableIndexes(const void* virtAddr, uint16_t& pageIndex, uint16_t& pageTableIndex,
										uint16_t& pageDirectoryIndex, uint16_t& pdptIndex)
{
	auto virtualAddress = reinterpret_cast<uintptr_t>(virtAddr);

	virtualAddress >>= 12;
	pageIndex = virtualAddress & 0b111'111'111;

	virtualAddress >>= 9;
	pageTableIndex = virtualAddress & 0b111'111'111;

	virtualAddress >>= 9;
	pageDirectoryIndex = virtualAddress & 0b111'111'111;

	virtualAddress >>= 9;
	pdptIndex = virtualAddress & 0b111'111'111;
}

void PagingManager::PopulatePagingStructureEntry(PagingStructureEntry* entry, uintptr_t physAddr, bool user)
{
	entry->SetPhysicalAddress(physAddr);

	// Flags
	entry->SetFlag(Present, true);
	entry->SetFlag(ReadWrite, true);
	entry->SetFlag(UserAllowed, user);
}

PagingStructure* PagingManager::AllocatePagingStructure(PagingStructureEntry* entry, bool user)
{
	auto physAddr = reinterpret_cast<uintptr_t>(RequestPageFrame());

	auto pagingStruct = reinterpret_cast<PagingStructure*>(HigherHalf(physAddr));
	Memset(pagingStruct, 0, 0x1000);

	PopulatePagingStructureEntry(entry, physAddr, user);

	return pagingStruct;
}

void PagingManager::InitializePaging()
{
    pml4PhysAddr = reinterpret_cast<uintptr_t>(RequestPageFrame());
    pml4 = reinterpret_cast<PagingStructure*>(HigherHalf(pml4PhysAddr));
    Memset(pml4, 0, 0x1000);

    for (size_t pageFrameIndex = 0; pageFrameIndex < pageFrameCount; ++pageFrameIndex)
    {
        uintptr_t physAddr = pageFrameIndex * 0x1000;
        MapMemory(reinterpret_cast<void*>(HigherHalf(physAddr)), reinterpret_cast<void*>(physAddr), false);
    }

    auto kernelBaseAddrStruct = reinterpret_cast<stivale2_struct_tag_kernel_base_address*>(GetStivale2Tag(STIVALE2_STRUCT_TAG_KERNEL_BASE_ADDRESS_ID));
    auto pmrsStruct = reinterpret_cast<stivale2_struct_tag_pmrs*>(GetStivale2Tag(STIVALE2_STRUCT_TAG_PMRS_ID));
    for (uint64_t pmrIndex = 0; pmrIndex < pmrsStruct->entries; ++pmrIndex)
    {
        stivale2_pmr pmr = pmrsStruct->pmrs[pmrIndex];
        for (uint64_t pmrVirtAddr = pmr.base; pmrVirtAddr < pmr.base + pmr.length; pmrVirtAddr += 0x1000)
        {
            uint64_t physAddr = kernelBaseAddrStruct->physical_base_address + (pmrVirtAddr - kernelBaseAddrStruct->virtual_base_address);
            MapMemory(reinterpret_cast<void*>(pmrVirtAddr), reinterpret_cast<void*>(physAddr), false);
        }
    }
}

void PagingManager::SetCR3() const
{
    asm volatile("mov %0, %%cr3" : : "r" (pml4PhysAddr));
}

void PagingManager::MapMemory(const void* virtAddr, const void* physAddr, bool user)
{
	uint16_t pageIndex, pageTableIndex, pageDirectoryIndex, pdptIndex;
	GetPageTableIndexes(virtAddr, pageIndex, pageTableIndex, pageDirectoryIndex, pdptIndex);

    PagingStructureEntry* pml4Entry = &pml4->entries[pdptIndex];
    PagingStructure* pdpt;
    if (!pml4Entry->GetFlag(Present))
    {
	    pdpt = AllocatePagingStructure(pml4Entry, user);
    }
    else
    {
        pdpt = reinterpret_cast<PagingStructure*>(HigherHalf(pml4Entry->GetPhysicalAddress()));
    }

    PagingStructureEntry* pdptEntry = &pdpt->entries[pageDirectoryIndex];
    PagingStructure* pageDirectory;
    if (!pdptEntry->GetFlag(Present))
    {
		pageDirectory = AllocatePagingStructure(pdptEntry, user);
    }
    else
    {
        pageDirectory = reinterpret_cast<PagingStructure*>(HigherHalf(pdptEntry->GetPhysicalAddress()));
    }

    PagingStructureEntry* pageDirectoryEntry = &pageDirectory->entries[pageTableIndex];
    PagingStructure* pageTable;
    if (!pageDirectoryEntry->GetFlag(Present))
    {
		pageTable = AllocatePagingStructure(pageDirectoryEntry, user);
    }
    else
    {
        pageTable = reinterpret_cast<PagingStructure*>(HigherHalf(pageDirectoryEntry->GetPhysicalAddress()));
    }

    PagingStructureEntry* pageTableEntry = &pageTable->entries[pageIndex];
	PopulatePagingStructureEntry(pageTableEntry, reinterpret_cast<uintptr_t>(physAddr), user);
}

void PagingManager::UnmapMemory(const void* virtAddr)
{
	uint16_t pageIndex, pageTableIndex, pageDirectoryIndex, pdptIndex;
	GetPageTableIndexes(virtAddr, pageIndex, pageTableIndex, pageDirectoryIndex, pdptIndex);

    auto pdpt = reinterpret_cast<PagingStructure*>(HigherHalf(pml4->entries[pdptIndex].GetPhysicalAddress()));
    auto pageDirectory = reinterpret_cast<PagingStructure*>(HigherHalf(pdpt->entries[pageDirectoryIndex].GetPhysicalAddress()));
    auto pageTable = reinterpret_cast<PagingStructure*>(HigherHalf(pageDirectory->entries[pageTableIndex].GetPhysicalAddress()));
    pageTable->entries[pageIndex].SetFlag(Present, false);

    asm volatile("invlpg (%0)" : : "b"(virtAddr) : "memory");
}

uint8_t PagingManager::PageNotPresentLevel(const void* virtAddr)
{
	uint16_t pageIndex, pageTableIndex, pageDirectoryIndex, pdptIndex;
	GetPageTableIndexes(virtAddr, pageIndex, pageTableIndex, pageDirectoryIndex, pdptIndex);

    PagingStructureEntry* pml4Entry = &pml4->entries[pdptIndex];
    if (!pml4Entry->GetFlag(Present)) return 4;

    auto pdpt = reinterpret_cast<PagingStructure*>(HigherHalf(pml4Entry->GetPhysicalAddress()));
    PagingStructureEntry* pdptEntry = &pdpt->entries[pageDirectoryIndex];
    if (!pdptEntry->GetFlag(Present)) return 3;

    auto pageDirectory = reinterpret_cast<PagingStructure*>(HigherHalf(pdptEntry->GetPhysicalAddress()));
    PagingStructureEntry* pageDirectoryEntry = &pageDirectory->entries[pageTableIndex];
    if (!pageDirectoryEntry->GetFlag(Present)) return 2;

    auto pageTable = reinterpret_cast<PagingStructure*>(HigherHalf(pageDirectoryEntry->GetPhysicalAddress()));
    PagingStructureEntry* pageTableEntry = &pageTable->entries[pageIndex];
    return !pageTableEntry->GetFlag(Present);
}

void PagingStructureEntry::SetFlag(PagingFlag flag, bool enable)
{
    if (enable) value |= (1 << flag);
    else value &= ~(1 << flag);
}

bool PagingStructureEntry::GetFlag(PagingFlag flag) const
{
    return value & (1 << flag);
}

void PagingStructureEntry::SetPhysicalAddress(uint64_t physAddr)
{
    physAddr &= 0x0000'fffffffff'000;
    value &=    0xffff'000000000'fff;
    value |= physAddr;
}

uint64_t PagingStructureEntry::GetPhysicalAddress() const
{
    return (value & 0x0000'fffffffff'000);
}