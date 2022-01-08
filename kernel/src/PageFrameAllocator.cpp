#include "PageFrameAllocator.h"

static Bitmap pageFrameBitmap;

uint64_t pageFrameCount = 0;

void InitializePageFrameAllocator()
{
    Serial::Print("Initializing bitmap page frame allocator (physical memory allocator)...");

    stivale2_struct_tag_memmap* memoryMapStruct = (stivale2_struct_tag_memmap*)GetStivale2Tag(STIVALE2_STRUCT_TAG_MEMMAP_ID);
    Serial::Printf("Memory Map provided by stivale2 contains %d entries.", memoryMapStruct->entries);

    stivale2_mmap_entry lastMemoryMapEntry = memoryMapStruct->memmap[memoryMapStruct->entries - 1];
    uint64_t memorySize = lastMemoryMapEntry.base + lastMemoryMapEntry.length;
    Serial::Printf("Memory size: %x", memorySize);

//    for (uint64_t entryIndex = 0; entryIndex < memoryMapTag->entries; ++entryIndex)
//    {
//        stivale2_mmap_entry memoryMapEntry = memoryMapTag->memmap[entryIndex];
//        Serial::Print("---------------");
//        Serial::Printf("Type: %x", memoryMapEntry.type);
//        Serial::Printf("Base: %x", memoryMapEntry.base);
//        Serial::Printf("Length: %x", memoryMapEntry.length);
//        Serial::Print("---------------");
//    }

    if (memorySize % 0x1000 != 0)
    {
        Serial::Print("Memory size is not aligned to page size (4096).");
        Serial::Print("Hanging...");
        while (true) asm("hlt");
    }

    pageFrameCount = memorySize / 0x1000;
    Serial::Printf("Physical memory contains %x page frames.", pageFrameCount);

    if (pageFrameCount % 8 != 0)
    {
        Serial::Print("Page frame count is not a multiple of 8.");
        Serial::Print("Hanging...");
        while (true) asm("hlt");
    }

    uint64_t bitmapSize = pageFrameCount / 8;
    pageFrameBitmap.size = bitmapSize;

    Serial::Print("Finding first usable memory section large enough to insert the page frame bitmap...");
    uint8_t* bitmapBuffer = NULL;
    for (uint64_t entryIndex = 0; entryIndex < memoryMapStruct->entries; ++entryIndex)
    {
        stivale2_mmap_entry memoryMapEntry = memoryMapStruct->memmap[entryIndex];
        if (memoryMapEntry.type == 1 && memoryMapEntry.length > bitmapSize)
        {
            bitmapBuffer = (uint8_t*)(memoryMapEntry.base + 0xffff'8000'0000'0000);
            Serial::Printf("Found usable memory section. Physical Base: %x", memoryMapEntry.base);
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
    Serial::Print("Setting all bits in the page frame bitmap...");
    Memset(bitmapBuffer, 0xff, bitmapSize);

    Serial::Print("Freeing usable page frames...");
    uint64_t freedUsablePageFrames = 0;
    for (uint64_t entryIndex = 0; entryIndex < memoryMapStruct->entries; ++entryIndex)
    {
        stivale2_mmap_entry memoryMapEntry = memoryMapStruct->memmap[entryIndex];

        if (memoryMapEntry.type == 1)
        {
            uint64_t basePageFrame = memoryMapEntry.base / 0x1000;
            for (uint64_t pageFrame = 0; pageFrame < memoryMapEntry.length / 0x1000; ++pageFrame)
            {
                pageFrameBitmap.SetBit(basePageFrame + pageFrame, false);
                freedUsablePageFrames++;
            }
        }
    }
    Serial::Printf("Freed %x usable page frames.", freedUsablePageFrames);

    Serial::Print("Locking page frames taken by the page frame bitmap...");
    uint64_t bitmapBasePageFrame = ((uint64_t)bitmapBuffer - 0xffff'8000'0000'0000) / 0x1000;
    for (uint64_t bitmapPage = 0; bitmapPage < bitmapSize / 0x1000; ++bitmapPage)
    {
        pageFrameBitmap.SetBit(bitmapBasePageFrame + bitmapPage, true);
    }

    Serial::Print("Completed initialization of page frame allocator (physical memory allocator).", "\n\n");
}

void* RequestPageFrame()
{
    // Find first free page frame, starting from page frame with the lowest physical address (0)
    for (uint64_t pageFrame = 0; pageFrame < pageFrameBitmap.size * 8; ++pageFrame)
    {
        if (!pageFrameBitmap.GetBit(pageFrame))
        {
            pageFrameBitmap.SetBit(pageFrame, true);
            return (void*)(pageFrame * 0x1000);
        }
    }

    // Free page frame could not be found.
    Serial::Print("Free page frame could not be found!");
    while (true) asm("hlt");
}