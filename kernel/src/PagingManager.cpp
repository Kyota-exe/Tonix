#include "PagingManager.h"

using namespace PageFrameAllocator;

void PagingManager::InitializePaging(stivale2_struct* stivale2Struct)
{
    Serial::Print("Initializing paging...");

    if (!initialized)
    {
        Serial::Print("Page frame allocator (physical memory allocator) must be initialized before initializing paging.");
        Serial::Print("Hanging...");
        while (true) asm("hlt");
    }

    Serial::Print("Allocating memory for PLM4...");
    uint64_t plm4PhysAddr = (uint64_t)RequestPageFrame();
    PagingStructure* plm4 = (PagingStructure*)(plm4PhysAddr + 0xffff'8000'0000'0000);
    Memset(plm4, 0, 0x1000);
    Serial::Printf("PLM4 Physical Address: %x", plm4PhysAddr);

    // Commented out for now since MapMemory prints a lot of stuff
    /*Serial::Print("Mapping all physical addresses to the virtual higher-half...");
    for (uint64_t pageFrameIndex = 0; pageFrameIndex < pageFrameCount; ++pageFrameIndex)
    {
        uint64_t physAddr = pageFrameIndex * 0x1000;
        MapMemory((void*)(physAddr + 0xffff'8000'0000'0000), (void*)physAddr);
    }*/

    Serial::Print("Mapping kernel to PMR...");
    stivale2_struct_tag_kernel_base_address* kernelBaseAddrTag = GetKernelAddressTag(stivale2Struct);
    stivale2_struct_tag_pmrs* pmrsTag = GetPMRs(stivale2Struct);
    Serial::Printf("PMR count: %d", pmrsTag->entries);
    Serial::Printf("Kernel physical base: %x", kernelBaseAddrTag->physical_base_address);
    Serial::Printf("Kernel virtual base: %x", kernelBaseAddrTag->virtual_base_address);
    for (uint64_t pmrIndex = 0; pmrIndex < pmrsTag->entries; ++pmrIndex)
    {
        stivale2_pmr pmr = pmrsTag->pmrs[pmrIndex];
//        Serial::Print("============================");
//        Serial::Printf("PMR base: %x", pmr.base);
//        Serial::Printf("PMR length: %x", pmr.length);
//        Serial::Print("============================");
        for (uint64_t pmrVirtAddr = pmr.base; pmrVirtAddr < pmr.base + pmr.length; pmrVirtAddr += 0x1000)
        {
            uint64_t offset = pmrVirtAddr - kernelBaseAddrTag->virtual_base_address;
            MapMemory((void*)pmrVirtAddr, (void*)(kernelBaseAddrTag->physical_base_address + offset));
        }
    }

    Serial::Print("\n\n\n\n\n");

    Serial::Printf("Moving value %x into CR3...", plm4PhysAddr);
    asm volatile("mov %0, %%cr3" : : "r" (plm4PhysAddr));
    uint64_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    Serial::Printf("New CR3 contents: %x", cr3);

    Serial::Print("Paging initialized.", "\n\n");S
    while (true) asm volatile("hlt");
}

void PagingManager::MapMemory(void* virtAddr, void* physAddr)
{
    Serial::Print("---------------------------------");
    Serial::Printf("Virt: %x", (uint64_t)virtAddr);
    Serial::Printf("Phys: %x", (uint64_t)physAddr);
    //Serial::Print("---------------------------------");

    uint64_t virtualAddress = (uint64_t)virtAddr;
    virtualAddress >>= 12;
    uint16_t pageIndex = virtualAddress & 0b111'111'111;
    virtualAddress >>= 9;
    uint16_t pageTableIndex = virtualAddress & 0b111'111'111;
    virtualAddress >>= 9;
    uint16_t pageDirectoryIndex = virtualAddress & 0b111'111'111;
    virtualAddress >>= 9;
    uint16_t pdptIndex = virtualAddress & 0b111'111'111;

    /*Serial::Printf("Page Index: %d", pageIndex);
    Serial::Printf("Page Table Index: %d", pageTableIndex);
    Serial::Printf("Page Directory Index: %d", pageDirectoryIndex);
    Serial::Printf("PDPT Index: %d", pdptIndex);*/

    PagingStructureEntry* plm4Entry = &(plm4->entries[pdptIndex]);
    PagingStructure* pdpt = NULL;
    if (!plm4Entry->GetFlag(Present))
    {
        uint64_t pdptPhysAddr = (uint64_t)RequestPageFrame();
        //Serial::Printf("Allocated page for new PDPT - PhysAddr = %x", pdptPhysAddr);
        pdpt = (PagingStructure*)(pdptPhysAddr + 0xffff'8000'0000'0000);
        Memset(pdpt, 0, 0x1000);
        plm4Entry->SetPhysicalAddress(pdptPhysAddr);

        // Flags
        plm4Entry->SetFlag(Present, true);
        plm4Entry->SetFlag(ReadWrite, true);
        //plm4Entry->SetFlag(UserAllowed, true);
    }
    else
    {
        pdpt = (PagingStructure*)(plm4Entry->GetPhysicalAddress() + 0xffff'8000'0000'0000);
    }

    PagingStructureEntry* pdptEntry = &(pdpt->entries[pageDirectoryIndex]);
    PagingStructure* pageDirectory = NULL;
    if (!pdptEntry->GetFlag(Present))
    {
        uint64_t pageDirectoryPhysAddr = (uint64_t)RequestPageFrame();
        //Serial::Printf("Allocated page for new PD - PhysAddr = %x", pageDirectoryPhysAddr);
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
        //Serial::Printf("Allocated page for new PT - PhysAddr = %x", pageTablePhysAddr);
        pageTable = (PagingStructure*)(pageTablePhysAddr + 0xffff'8000'0000'0000);
        Memset(pageTable, 0, 0x1000);
        pageDirectoryEntry->SetPhysicalAddress(pageTablePhysAddr);

        // Flags
        pageDirectoryEntry->SetFlag(Present, true);
        pageDirectoryEntry->SetFlag(ReadWrite, true);
        //pageDirectoryEntry->SetFlag(UserAllowed, true);
    }
    else
    {
        pageTable = (PagingStructure*)(pageDirectoryEntry->GetPhysicalAddress() + 0xffff'8000'0000'0000);
    }

    PagingStructureEntry* pageTableEntry = &pageTable->entries[pageIndex];
    pageTableEntry->SetPhysicalAddress((uint64_t)physAddr);
    pageTableEntry->SetFlag(Present, true);
    pageTableEntry->SetFlag(ReadWrite, true);
    //pageTableEntry->SetFlag(UserAllowed, true);

    Serial::Printf("PML4[%d]: ", pdptIndex, "");
    Serial::Printf("%x", plm4->entries[pdptIndex].value);
    Serial::Printf("PDPT[%d]: ", pageDirectoryIndex, "");
    Serial::Printf("%x", pdpt->entries[pageDirectoryIndex].value);
    Serial::Printf("PD[%d]: ", pageTableIndex, "");
    Serial::Printf("%x",pageDirectory->entries[pageTableIndex].value);
    Serial::Printf("PT[%d]: ", pageIndex, "");
    Serial::Printf("%x", pageTable->entries[pageIndex].value);
    Serial::Print("---------------------------------");
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
    uint64_t nonCanonicalPart = physAddr >> 48;
    if (nonCanonicalPart != 0 && nonCanonicalPart != 0xffff)
    {
        Serial::Print("Cannot set non-canonical physical address to physAddr in a page structure struct.");
        Serial::Printf("Address: %x", physAddr);
        Serial::Print("Hanging...");
        while (true) asm("hlt");
    }
    physAddr &= 0x0000'fffffffff'000;
    value &=    0xffff'000000000'fff;
    value |= physAddr;
}

uint64_t PagingStructureEntry::GetPhysicalAddress()
{
    return (value & 0x0000'fffffffff'000);
}