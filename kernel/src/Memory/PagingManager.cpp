#include "Memory/Memory.h"
#include "Memory/PageFrameAllocator.h"
#include "Memory/PagingManager.h"
#include "Stivale2Interface.h"

void PagingManager::InitializePaging()
{
    pml4PhysAddr = (uint64_t)RequestPageFrame();
    pml4 = (PagingStructure*)(pml4PhysAddr + 0xffff'8000'0000'0000);
    Memset(pml4, 0, 0x1000);

    for (uint64_t pageFrameIndex = 0; pageFrameIndex < pageFrameCount; ++pageFrameIndex)
    {
        uint64_t physAddr = pageFrameIndex * 0x1000;
        MapMemory((void*)(physAddr + 0xffff'8000'0000'0000), (void*)physAddr, false);
    }

    auto kernelBaseAddrStruct = (stivale2_struct_tag_kernel_base_address*)GetStivale2Tag(STIVALE2_STRUCT_TAG_KERNEL_BASE_ADDRESS_ID);
    auto pmrsStruct = (stivale2_struct_tag_pmrs*)GetStivale2Tag(STIVALE2_STRUCT_TAG_PMRS_ID);
    for (uint64_t pmrIndex = 0; pmrIndex < pmrsStruct->entries; ++pmrIndex)
    {
        stivale2_pmr pmr = pmrsStruct->pmrs[pmrIndex];
        for (uint64_t pmrVirtAddr = pmr.base; pmrVirtAddr < pmr.base + pmr.length; pmrVirtAddr += 0x1000)
        {
            uint64_t physAddr = kernelBaseAddrStruct->physical_base_address + (pmrVirtAddr - kernelBaseAddrStruct->virtual_base_address);
            MapMemory((void*)pmrVirtAddr, (void*)physAddr, false);
        }
    }
}

void PagingManager::SetCR3() const
{
    asm volatile("mov %0, %%cr3" : : "r" (pml4PhysAddr));
}

void PagingManager::MapMemory(void* virtAddr, void* physAddr, bool user)
{
    auto virtualAddress = (uint64_t)virtAddr;
    virtualAddress >>= 12;
    uint16_t pageIndex = virtualAddress & 0b111'111'111;
    virtualAddress >>= 9;
    uint16_t pageTableIndex = virtualAddress & 0b111'111'111;
    virtualAddress >>= 9;
    uint16_t pageDirectoryIndex = virtualAddress & 0b111'111'111;
    virtualAddress >>= 9;
    uint16_t pdptIndex = virtualAddress & 0b111'111'111;

    PagingStructureEntry* pml4Entry = &pml4->entries[pdptIndex];
    PagingStructure* pdpt;
    if (!pml4Entry->GetFlag(Present))
    {
        auto pdptPhysAddr = (uint64_t)RequestPageFrame();
        pdpt = (PagingStructure*)(pdptPhysAddr + 0xffff'8000'0000'0000);
        Memset(pdpt, 0, 0x1000);
        pml4Entry->SetPhysicalAddress(pdptPhysAddr);

        // Flags
        pml4Entry->SetFlag(Present, true);
        pml4Entry->SetFlag(ReadWrite, true);
        pml4Entry->SetFlag(UserAllowed, user);
    }
    else
    {
        pdpt = (PagingStructure*)(pml4Entry->GetPhysicalAddress() + 0xffff'8000'0000'0000);
    }

    PagingStructureEntry* pdptEntry = &pdpt->entries[pageDirectoryIndex];
    PagingStructure* pageDirectory;
    if (!pdptEntry->GetFlag(Present))
    {
        auto pageDirectoryPhysAddr = (uint64_t)RequestPageFrame();
        pageDirectory = (PagingStructure*)(pageDirectoryPhysAddr + 0xffff'8000'0000'0000);
        Memset(pageDirectory, 0, 0x1000);
        pdptEntry->SetPhysicalAddress(pageDirectoryPhysAddr);

        // Flags
        pdptEntry->SetFlag(Present, true);
        pdptEntry->SetFlag(ReadWrite, true);
        pdptEntry->SetFlag(UserAllowed, user);
    }
    else
    {
        pageDirectory = (PagingStructure*)(pdptEntry->GetPhysicalAddress() + 0xffff'8000'0000'0000);
    }

    PagingStructureEntry* pageDirectoryEntry = &pageDirectory->entries[pageTableIndex];
    PagingStructure* pageTable;
    if (!pageDirectoryEntry->GetFlag(Present))
    {
        auto pageTablePhysAddr = (uint64_t)RequestPageFrame();
        pageTable = (PagingStructure*)(pageTablePhysAddr + 0xffff'8000'0000'0000);
        Memset(pageTable, 0, 0x1000);
        pageDirectoryEntry->SetPhysicalAddress(pageTablePhysAddr);

        // Flags
        pageDirectoryEntry->SetFlag(Present, true);
        pageDirectoryEntry->SetFlag(ReadWrite, true);
        pageDirectoryEntry->SetFlag(UserAllowed, user);
    }
    else
    {
        pageTable = (PagingStructure*)(pageDirectoryEntry->GetPhysicalAddress() + 0xffff'8000'0000'0000);
    }

    PagingStructureEntry* pageTableEntry = &pageTable->entries[pageIndex];
    pageTableEntry->SetPhysicalAddress((uint64_t)physAddr);
    pageTableEntry->SetFlag(Present, true);
    pageTableEntry->SetFlag(ReadWrite, true);
    pageTableEntry->SetFlag(UserAllowed, user);
}

void PagingManager::UnmapMemory(void* virtAddr)
{
    auto virtualAddress = (uint64_t)virtAddr;
    virtualAddress >>= 12;
    uint16_t pageIndex = virtualAddress & 0b111'111'111;
    virtualAddress >>= 9;
    uint16_t pageTableIndex = virtualAddress & 0b111'111'111;
    virtualAddress >>= 9;
    uint16_t pageDirectoryIndex = virtualAddress & 0b111'111'111;
    virtualAddress >>= 9;
    uint16_t pdptIndex = virtualAddress & 0b111'111'111;

    auto pdpt = (PagingStructure*)(pml4->entries[pdptIndex].GetPhysicalAddress() + 0xffff'8000'0000'0000);
    auto pageDirectory = (PagingStructure*)(pdpt->entries[pageDirectoryIndex].GetPhysicalAddress() + 0xffff'8000'0000'0000);
    auto pageTable = (PagingStructure*)(pageDirectory->entries[pageTableIndex].GetPhysicalAddress() + 0xffff'8000'0000'0000);
    pageTable->entries[pageIndex].SetFlag(Present, false);

    asm volatile("invlpg (%0)" : : "b"(virtAddr) : "memory");
}

uint8_t PagingManager::CheckIfPagePresent(void* virtAddr)
{
    auto virtualAddress = (uint64_t)virtAddr;
    virtualAddress >>= 12;
    uint16_t pageIndex = virtualAddress & 0b111'111'111;
    virtualAddress >>= 9;
    uint16_t pageTableIndex = virtualAddress & 0b111'111'111;
    virtualAddress >>= 9;
    uint16_t pageDirectoryIndex = virtualAddress & 0b111'111'111;
    virtualAddress >>= 9;
    uint16_t pdptIndex = virtualAddress & 0b111'111'111;

    PagingStructureEntry* pml4Entry = &pml4->entries[pdptIndex];
    if (!pml4Entry->GetFlag(Present)) return 4;

    auto pdpt = (PagingStructure*)(pml4Entry->GetPhysicalAddress() + 0xffff'8000'0000'0000);
    PagingStructureEntry* pdptEntry = &pdpt->entries[pageDirectoryIndex];
    if (!pdptEntry->GetFlag(Present)) return 3;

    auto pageDirectory = (PagingStructure*)(pdptEntry->GetPhysicalAddress() + 0xffff'8000'0000'0000);
    PagingStructureEntry* pageDirectoryEntry = &pageDirectory->entries[pageTableIndex];
    if (!pageDirectoryEntry->GetFlag(Present)) return 2;

    auto pageTable = (PagingStructure*)(pageDirectoryEntry->GetPhysicalAddress() + 0xffff'8000'0000'0000);
    PagingStructureEntry* pageTableEntry = &pageTable->entries[pageIndex];
    return (!pageTableEntry->GetFlag(Present));
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