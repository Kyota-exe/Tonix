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
    uint64_t plm4PhysAddr = (uint64_t)RequestPage();
    PLM4* plm4 = (PLM4*)(plm4PhysAddr + 0xffff'8000'0000'0000);
    Memset(plm4, 0, 0x1000);
    Serial::Printf("PLM4 Physical Address: %x", (uint64_t)plm4PhysAddr);

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
        Serial::Print("============================");
        Serial::Printf("PMR base: %x", pmr.base);
        Serial::Printf("PMR length: %x", pmr.length);
        Serial::Print("============================");
//        for (uint64_t pmrVirtAddr = pmr.base; pmrVirtAddr < pmr.base + pmr.length; pmrVirtAddr += 0x1000)
//        {
//            uint64_t offset = pmrVirtAddr - kernelBaseAddrTag->virtual_base_address;
//            MapMemory((void*)pmrVirtAddr, (void*)(kernelBaseAddrTag->physical_base_address + offset));
//        }
        for (uint64_t pageIndex = 0; pageIndex < pmr.length / 0x1000; ++pageIndex)
        {
            uint64_t offset = pmr.base + pageIndex * 0x1000 - kernelBaseAddrTag->virtual_base_address;
            MapMemory((void*)(pmr.base + pageIndex * 0x1000), (void*)(kernelBaseAddrTag->physical_base_address + offset));
        }
    }

    asm volatile("mov %0, %%cr3" : : "r" (plm4PhysAddr));

    Serial::Print("Paging initialized.", "\n\n");
    while (true) asm volatile("hlt");
}

void PagingManager::MapMemory(void* virtAddr, void* physAddr)
{
    Serial::Print("---------------------------------");
    Serial::Printf("Virt: %x", (uint64_t)virtAddr);
    Serial::Printf("Phys: %x", (uint64_t)physAddr);
    Serial::Print("---------------------------------");

    uint64_t virtualAddress = (uint64_t)virtAddr;
    virtualAddress >>= 12;
    uint16_t pageIndex = virtualAddress & 0b111'111'111;
    virtualAddress >>= 9;
    uint16_t pageTableIndex = virtualAddress & 0b111'111'111;
    virtualAddress >>= 9;
    uint16_t pageDirectoryIndex = virtualAddress & 0b111'111'111;
    virtualAddress >>= 9;
    uint16_t pdptIndex = virtualAddress & 0b111'111'111;

    Serial::Printf("Page Index: %d", pageIndex);
    Serial::Printf("Page Table Index: %d", pageTableIndex);
    Serial::Printf("Page Directory Index: %d", pageDirectoryIndex);
    Serial::Printf("PDPT Index: %d", pdptIndex);

    PLM4Entry* plm4Entry = &plm4->entries[pdptIndex];
    PDPT* pdpt = NULL;
    if (!plm4Entry->present)
    {
        uint64_t pdptPhysAddr = (uint64_t)RequestPage();
        pdpt = (PDPT*)(pdptPhysAddr + 0xffff'8000'0000'0000);
        Memset(pdpt, 0, 0x1000);
        plm4Entry->pdptPhysAddr = pdptPhysAddr >> 12;

        // Flags
        plm4Entry->present = true;
        plm4Entry->readWrite = true;
        plm4Entry->userSupervisor = true;
    }
    else
    {
        pdpt = (PDPT*)((plm4Entry->pdptPhysAddr << 12) + 0xffff'8000'0000'0000);
    }

    PDPTEntry* pdptEntry = &pdpt->entries[pageDirectoryIndex];
    PageDirectory* pageDirectory = NULL;
    if (!pdptEntry->present)
    {
        uint64_t pageDirectoryPhysAddr = (uint64_t)RequestPage();
        pageDirectory = (PageDirectory*)(pageDirectoryPhysAddr + 0xffff'8000'0000'0000);
        Memset(pageDirectory, 0, 0x1000);
        pdptEntry->pageDirectoryPhysAddr = pageDirectoryPhysAddr >> 12;

        // Flags
        pdptEntry->present = true;
        pdptEntry->readWrite = true;
        pdptEntry->userSupervisor = true;
    }
    else
    {
        pageDirectory = (PageDirectory*)((pdptEntry->pageDirectoryPhysAddr << 12) + 0xffff'8000'0000'0000);
    }

    PageDirectoryEntry* pageDirectoryEntry = &pageDirectory->entries[pageTableIndex];
    PageTable* pageTable = NULL;
    if (!pageDirectoryEntry->present)
    {
        uint64_t pageTablePhysAddr = (uint64_t)RequestPage();
        pageTable = (PageTable*)(pageTablePhysAddr + 0xffff'8000'0000'0000);
        Memset(pageTable, 0, 0x1000);
        pageDirectoryEntry->pageTablePhysAddr = pageTablePhysAddr >> 12;

        // Flags
        pageDirectoryEntry->present = true;
        pageDirectoryEntry->readWrite = true;
        pageDirectoryEntry->userSupervisor = true;
    }
    else
    {
        pageTable = (PageTable*)((pageDirectoryEntry->pageTablePhysAddr << 12) + 0xffff'8000'0000'0000);
    }

    PageTableEntry* page = &pageTable->entries[pageIndex];
    page->pageFramePhysAddr = (uint64_t)physAddr >> 12;
    page->present = true;
    page->readWrite = true;
    page->userSupervisor = true;
}