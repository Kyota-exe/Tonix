#include "Memory/Memory.h"
#include "Memory/PageFrameAllocator.h"
#include "Memory/PagingManager.h"
#include "Stivale2Interface.h"

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

void PagingManager::PopulatePagingStructureEntry(PageTableEntry* entry, uintptr_t physAddr, bool user)
{
	entry->SetPhysicalAddress(physAddr);

	// Flags
	entry->SetFlag(PagingFlag::Present, true);
	entry->SetFlag(PagingFlag::ReadWrite, true);
	entry->SetFlag(PagingFlag::UserAllowed, user);
}

PageTable* PagingManager::AllocatePagingStructure(PageTableEntry* entry, bool user)
{
	auto physAddr = reinterpret_cast<uintptr_t>(RequestPageFrame());

	auto pagingStruct = reinterpret_cast<PageTable*>(HigherHalf(physAddr));
	Memset(pagingStruct, 0, 0x1000);

	PopulatePagingStructureEntry(entry, physAddr, user);

	return pagingStruct;
}

void PagingManager::InitializePaging()
{
    pml4PhysAddr = reinterpret_cast<uintptr_t>(RequestPageFrame());
    pml4 = reinterpret_cast<PageTable*>(HigherHalf(pml4PhysAddr));
    Memset(pml4, 0, 0x1000);

    for (uint64_t pageFrameIndex = 0; pageFrameIndex < pageFrameCount; ++pageFrameIndex)
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
	uint16_t pageIndexes[PAGING_LEVELS];
	GetPageTableIndexes(virtAddr, pageIndexes);

	PageTable* table = pml4;
	for (unsigned int level = PAGING_LEVELS - 1; level > 0; --level)
	{
		auto entry = &table->entries[pageIndexes[level]];
		if (entry->GetFlag(PagingFlag::Present))
		{
			table = reinterpret_cast<PageTable*>(HigherHalf(entry->GetPhysicalAddress()));
		}
		else
		{
			table = AllocatePagingStructure(entry, user);
		}
	}

	PopulatePagingStructureEntry(&table->entries[pageIndexes[0]], reinterpret_cast<uintptr_t>(physAddr), user);
}

void PagingManager::UnmapMemory(const void* virtAddr)
{
	uint16_t pageIndexes[PAGING_LEVELS];
	GetPageTableIndexes(virtAddr, pageIndexes);

	PageTable* table = pml4;
	for (unsigned int level = PAGING_LEVELS - 1; level > 0; --level)
	{
		auto i = pageIndexes[level];
		PageTableEntry entry = table->entries[i];
		if (!entry.GetFlag(PagingFlag::Present)) return;
		table = reinterpret_cast<PageTable*>(HigherHalf(entry.GetPhysicalAddress()));
	}

    table->entries[pageIndexes[0]].SetFlag(PagingFlag::Present, false);
    asm volatile("invlpg (%0)" : : "b"(virtAddr) : "memory");
}

unsigned int PagingManager::FlagMismatchLevel(const void* virtAddr, PagingFlag flag, bool enabled)
{
	uint16_t pageIndexes[PAGING_LEVELS];
	GetPageTableIndexes(virtAddr, pageIndexes);

	PageTable* table = pml4;
	for (unsigned int level = PAGING_LEVELS - 1; level > 0; --level)
	{
		auto i = pageIndexes[level];
		auto entry = table->entries[i];
		if (!entry.GetFlag(flag))
		{
			return level;
		}
		table = reinterpret_cast<PageTable*>(HigherHalf(entry.GetPhysicalAddress()));
	}

	return 0;
}

void PageTableEntry::SetFlag(PagingFlag flag, bool enable)
{
    if (enable) value |= (1 << (uint64_t)flag);
    else value &= ~(1 << (uint64_t)flag);
}

bool PageTableEntry::GetFlag(PagingFlag flag) const
{
    return value & (1 << (uint64_t)flag);
}

void PageTableEntry::SetPhysicalAddress(uint64_t physAddr)
{
    physAddr &= 0x0000'fffffffff'000;
    value &=    0xffff'000000000'fff;
    value |= physAddr;
}

uint64_t PageTableEntry::GetPhysicalAddress() const
{
    return value & 0x0000'fffffffff'000;
}