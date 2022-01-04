#include "PageFrameAllocator.h"

static Bitmap pageFrameBitmap;

bool PageFrameAllocator::initialized = false;
uint64_t PageFrameAllocator::pageFrameCount = 0;

void PageFrameAllocator::InitializePageFrameAllocator(stivale2_struct *stivale2Struct)
{
    Serial::Print("Initializing bitmap page frame allocator (physical memory allocator)...");

    stivale2_struct_tag_memmap* memoryMapTag = GetMemoryMap(stivale2Struct);
    Serial::Printf("Memory Map provided by stivale2 contains %d entries.", memoryMapTag->entries);

    stivale2_mmap_entry lastMemoryMapEntry = memoryMapTag->memmap[memoryMapTag->entries - 1];
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

    uint64_t bitmapSize = pageFrameCount / 8 + (pageFrameCount % 8 == 0 ? 0 : 1);
    Serial::Printf("Page Frame Bitmap Size: %x", bitmapSize);
    pageFrameBitmap.size = bitmapSize;

    Serial::Printf("Number of padding bits at the end of the page frame bitmap: %d", (pageFrameCount % 8 == 0) ? 0 : (8 - pageFrameCount % 8));

    Serial::Print("Finding first usable memory section large enough to insert the page frame bitmap...");
    uint8_t* bitmapBuffer = NULL;
    for (uint64_t entryIndex = 0; entryIndex < memoryMapTag->entries; ++entryIndex)
    {
        stivale2_mmap_entry memoryMapEntry = memoryMapTag->memmap[entryIndex];
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
    for (uint64_t entryIndex = 0; entryIndex < memoryMapTag->entries; ++entryIndex)
    {
        stivale2_mmap_entry memoryMapEntry = memoryMapTag->memmap[entryIndex];

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
    for (uint64_t bitmapPage = 0; bitmapPage < bitmapSize / 0x1000 + 1; ++bitmapPage)
    {
        pageFrameBitmap.SetBit(bitmapBasePageFrame + bitmapPage, true);
    }

    initialized = true;
    Serial::Print("Completed initialization of page frame allocator (physical memory allocator).", "\n\n");
}

void* PageFrameAllocator::RequestPageFrame()
{
    if (!initialized)
    {
        Serial::Print("Page frame allocator (physical memory allocator) must be initialized before requesting pages.");
        Serial::Print("Hanging...");
        while (true) asm("hlt");
    }

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