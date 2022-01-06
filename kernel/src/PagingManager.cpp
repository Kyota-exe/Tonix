#include "PagingManager.h"

void PagingManager::InitializePaging()
{
    Serial::Print("Initializing paging...");

    Serial::Print("Allocating memory for PML4...");
    uint64_t pml4PhysAddr = (uint64_t)RequestPageFrame();
    pml4 = (PagingStructure*)(pml4PhysAddr + 0xffff'8000'0000'0000);
    Memset(pml4, 0, 0x1000);
    Serial::Printf("PML4 Physical Address: %x", pml4PhysAddr);

    Serial::Print("Mapping all physical addresses to the virtual higher-half...");
    for (uint64_t pageFrameIndex = 0; pageFrameIndex < pageFrameCount; ++pageFrameIndex)
    {
        uint64_t physAddr = pageFrameIndex * 0x1000;
        MapMemory((void*)(physAddr + 0xffff'8000'0000'0000), (void*)physAddr);
    }

    Serial::Print("Mapping kernel to PMR...");
    stivale2_struct_tag_kernel_base_address* kernelBaseAddrTag = (stivale2_struct_tag_kernel_base_address*)GetStivale2Tag(STIVALE2_STRUCT_TAG_KERNEL_BASE_ADDRESS_ID);
    stivale2_struct_tag_pmrs* pmrsTag = (stivale2_struct_tag_pmrs*)GetStivale2Tag(STIVALE2_STRUCT_TAG_PMRS_ID);
    Serial::Printf("PMR count: %d", pmrsTag->entries);
    Serial::Printf("Kernel physical base: %x", kernelBaseAddrTag->physical_base_address);
    Serial::Printf("Kernel virtual base: %x", kernelBaseAddrTag->virtual_base_address);
    for (uint64_t pmrIndex = 0; pmrIndex < pmrsTag->entries; ++pmrIndex)
    {
        stivale2_pmr pmr = pmrsTag->pmrs[pmrIndex];
        for (uint64_t pmrVirtAddr = pmr.base; pmrVirtAddr < pmr.base + pmr.length; pmrVirtAddr += 0x1000)
        {
            uint64_t offset = pmrVirtAddr - kernelBaseAddrTag->virtual_base_address;
            MapMemory((void*)pmrVirtAddr, (void*)(kernelBaseAddrTag->physical_base_address + offset));
        }
    }

    Serial::Printf("Moving the PML4's physical address into CR3...", pml4PhysAddr);
    asm volatile("mov %0, %%cr3" : : "r" (pml4PhysAddr));

    Serial::Print("Paging initialized.", "\n\n");
}

void PagingManager::MapMemory(void* virtAddr, void* physAddr)
{
    uint64_t virtualAddress = (uint64_t)virtAddr;
    virtualAddress >>= 12;
    uint16_t pageIndex = virtualAddress & 0b111'111'111;
    virtualAddress >>= 9;
    uint16_t pageTableIndex = virtualAddress & 0b111'111'111;
    virtualAddress >>= 9;
    uint16_t pageDirectoryIndex = virtualAddress & 0b111'111'111;
    virtualAddress >>= 9;
    uint16_t pdptIndex = virtualAddress & 0b111'111'111;

    PagingStructureEntry* pml4Entry = &pml4->entries[pdptIndex];
    PagingStructure* pdpt = NULL;
    if (!pml4Entry->GetFlag(Present))
    {
        uint64_t pdptPhysAddr = (uint64_t)RequestPageFrame();
        pdpt = (PagingStructure*)(pdptPhysAddr + 0xffff'8000'0000'0000);
        Memset(pdpt, 0, 0x1000);
        pml4Entry->SetPhysicalAddress(pdptPhysAddr);

        // Flags
        pml4Entry->SetFlag(Present, true);
        pml4Entry->SetFlag(ReadWrite, true);
    }
    else
    {
        pdpt = (PagingStructure*)(pml4Entry->GetPhysicalAddress() + 0xffff'8000'0000'0000);
    }

    PagingStructureEntry* pdptEntry = &pdpt->entries[pageDirectoryIndex];
    PagingStructure* pageDirectory = NULL;
    if (!pdptEntry->GetFlag(Present))
    {
        uint64_t pageDirectoryPhysAddr = (uint64_t)RequestPageFrame();
        pageDirectory = (PagingStructure*)(pageDirectoryPhysAddr + 0xffff'8000'0000'0000);
        Memset(pageDirectory, 0, 0x1000);
        pdptEntry->SetPhysicalAddress(pageDirectoryPhysAddr);

        // Flags
        pdptEntry->SetFlag(Present, true);
        pdptEntry->SetFlag(ReadWrite, true);
        //pdptEntry->SetFlag(UserAllowed, true);
    }
    else
    {
        pageDirectory = (PagingStructure*)(pdptEntry->GetPhysicalAddress() + 0xffff'8000'0000'0000);
    }

    PagingStructureEntry* pageDirectoryEntry = &pageDirectory->entries[pageTableIndex];
    PagingStructure* pageTable = NULL;
    if (!pageDirectoryEntry->GetFlag(Present))
    {
        uint64_t pageTablePhysAddr = (uint64_t)RequestPageFrame();
        pageTable = (PagingStructure*)(pageTablePhysAddr + 0xffff'8000'0000'0000);
        Memset(pageTable, 0, 0x1000);
        pageDirectoryEntry->SetPhysicalAddress(pageTablePhysAddr);

        // Flags
        pageDirectoryEntry->SetFlag(Present, true);
        pageDirectoryEntry->SetFlag(ReadWrite, true);
    }
    else
    {
        pageTable = (PagingStructure*)(pageDirectoryEntry->GetPhysicalAddress() + 0xffff'8000'0000'0000);
    }

    PagingStructureEntry* pageTableEntry = &pageTable->entries[pageIndex];
    pageTableEntry->SetPhysicalAddress((uint64_t)physAddr);
    pageTableEntry->SetFlag(Present, true);
    pageTableEntry->SetFlag(ReadWrite, true);
}

void PagingManager::UnmapMemory(void* virtAddr)
{
    uint64_t virtualAddress = (uint64_t)virtAddr;
    virtualAddress >>= 12;
    uint16_t pageIndex = virtualAddress & 0b111'111'111;
    virtualAddress >>= 9;
    uint16_t pageTableIndex = virtualAddress & 0b111'111'111;
    virtualAddress >>= 9;
    uint16_t pageDirectoryIndex = virtualAddress & 0b111'111'111;
    virtualAddress >>= 9;
    uint16_t pdptIndex = virtualAddress & 0b111'111'111;

    PagingStructure* pdpt = (PagingStructure*)(pml4->entries[pdptIndex].GetPhysicalAddress() + 0xffff'8000'0000'0000);
    PagingStructure* pageDirectory = (PagingStructure*)(pdpt->entries[pageDirectoryIndex].GetPhysicalAddress() + 0xffff'8000'0000'0000);
    PagingStructure* pageTable = (PagingStructure*)(pageDirectory->entries[pageTableIndex].GetPhysicalAddress() + 0xffff'8000'0000'0000);
    pageTable->entries[pageIndex].SetFlag(Present, false);

    asm volatile("invlpg (%0)" : : "b"(virtAddr) : "memory");
}

void PagingStructureEntry::SetFlag(PagingFlag flag, bool enable)
{
    if (enable) value |= (1 << flag);
    else value &= ~(1 << flag);
}

bool PagingStructureEntry::GetFlag(PagingFlag flag)
{
    return value & (1 << flag);
}

void PagingStructureEntry::SetPhysicalAddress(uint64_t physAddr)
{
    physAddr &= 0x0000'fffffffff'000;
    value &=    0xffff'000000000'fff;
    value |= physAddr;
}

uint64_t PagingStructureEntry::GetPhysicalAddress()
{
    return (value & 0x0000'fffffffff'000);
}