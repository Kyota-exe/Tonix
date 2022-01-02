#include "PageFrameAllocator.h"

Bitmap pageFrameBitmap;

void InitializePageFrameAllocator(stivale2_struct *stivale2Struct)
{
    Serial::Print("Initializing bitmap page frame allocator (physical memory allocator)...");

    stivale2_struct_tag_memmap* memoryMapTag = GetMemoryMap(stivale2Struct);
    Serial::Printf("Memory Map provided by stivale2 contains %d entries.", memoryMapTag->entries);

    Serial::Print("Finding last usable byte...");
    uint64_t lastUsableByte = 0;
    for (uint64_t entryIndex = 0; entryIndex < memoryMapTag->entries; ++entryIndex)
    {
        stivale2_mmap_entry memoryMapEntry = memoryMapTag->memmap[entryIndex];

        /*Serial::Print("---------------");
        Serial::Printf("Type: %x", memoryMapEntry.type);
        Serial::Printf("Base: %x", memoryMapEntry.base);
        Serial::Printf("Length: %x", memoryMapEntry.length);
        Serial::Print("---------------");*/

        // 1 for usable memory, 0x1000 for bootloader reclaimable memory
        if (memoryMapEntry.type == 1 || memoryMapEntry.type == 0x1000)
        {
            lastUsableByte = memoryMapEntry.base + memoryMapEntry.length;
        }
    }

    if (lastUsableByte % 0x1000 != 0)
    {
        Serial::Print("Last usable byte address is not aligned to page size (4096).");
        Serial::Print("Hanging...");
        while (true) asm("hlt");
    }

    if (lastUsableByte == 0)
    {
        Serial::Print("Could not find usable or bootloader reclaimable memory.");
        Serial::Print("Hanging...");
        while (true) asm("hlt");
    }

    Serial::Printf("Last usable byte: %x", lastUsableByte);
    uint64_t pageFrameCount = lastUsableByte / 0x1000;
    Serial::Printf("Physical memory contains %x page frames, excluding reserved page frames at the end.", pageFrameCount);
    uint64_t bitmapSize = pageFrameCount / 8 + 1;
    Serial::Printf("Page Frame Bitmap Size: %x", bitmapSize);

    Serial::Print("Finding first usable memory section large enough to insert page frame bitmap...");
    uint8_t* bitmapBuffer = NULL;
    for (uint64_t entryIndex = 0; entryIndex < memoryMapTag->entries; ++entryIndex)
    {
        stivale2_mmap_entry memoryMapEntry = memoryMapTag->memmap[entryIndex];
        if ((memoryMapEntry.type == 1 || memoryMapEntry.type == 0x1000) && memoryMapEntry.length > bitmapSize)
        {
            bitmapBuffer = (uint8_t*)memoryMapEntry.base;
            Serial::Print("Found usable memory section.");
            Serial::Printf("Base: %x", memoryMapEntry.base);
            break;
        }
    }

    if (bitmapBuffer == NULL)
    {
        Serial::Print("Could not find usable memory section large enough to insert page frame bitmap.");
        Serial::Print("Hanging...");
        while (true) asm("hlt");
    }

    pageFrameBitmap.buffer = bitmapBuffer;

    Serial::Print("Locking reserved page frames...");
    for (uint64_t entryIndex = 0; entryIndex < memoryMapTag->entries; ++entryIndex)
    {
        stivale2_mmap_entry memoryMapEntry = memoryMapTag->memmap[entryIndex];
        pageFrameBitmap.SetBit(memoryMapEntry.base / 0x1000, memoryMapEntry.type != 1 && memoryMapEntry.type != 0x1000);
    }

    Serial::Print("Locking page frames taken by the page frame bitmap...");
    for (uint64_t bitmapPage = 0; bitmapPage < bitmapSize / 0x1000 + 1; ++bitmapPage)
    {
        pageFrameBitmap.SetBit(bitmapPage, true);
    }

    Serial::Print("Completed initialization of page frame allocator (physical memory allocator).", "\n\n");
}