#include "Memory/Memory.h"
#include "Memory/PageFrameAllocator.h"
#include "Serial.h"
#include "Stivale2Interface.h"
#include "Panic.h"
#include "Bitmap.h"

static Bitmap pageFrameBitmap;

uint64_t pageFrameCount = 0;
uint64_t latestAllocatedPageFrame = 0;

void InitializePageFrameAllocator()
{
    auto memoryMapStruct = (stivale2_struct_tag_memmap*)GetStivale2Tag(STIVALE2_STRUCT_TAG_MEMMAP_ID);
    stivale2_mmap_entry lastMemoryMapEntry = memoryMapStruct->memmap[memoryMapStruct->entries - 1];
    uint64_t memorySize = lastMemoryMapEntry.base + lastMemoryMapEntry.length;
    Serial::Printf("Memory size: %x", memorySize);

//    for (uint64_t entryIndex = 0; entryIndex < memoryMapStruct->entries; ++entryIndex)
//    {
//        stivale2_mmap_entry memoryMapEntry = memoryMapStruct->memmap[entryIndex];
//        Serial::Print("---------------");
//        Serial::Printf("Type: %x", memoryMapEntry.type);
//        Serial::Printf("Base: %x", memoryMapEntry.base);
//        Serial::Printf("Length: %x", memoryMapEntry.length);
//        Serial::Print("---------------");
//    }

    KAssert(memorySize % 0x1000 == 0, "Memory size is not aligned to page size (4096).");

    pageFrameCount = memorySize / 0x1000;
    Serial::Printf("Physical memory contains %x page frames.", pageFrameCount);

    KAssert(pageFrameCount % 8 == 0, "Page frame count is not a multiple of 8.");

    uint64_t bitmapSize = pageFrameCount / 8;
    pageFrameBitmap.size = bitmapSize;

    uint8_t* bitmapBuffer = nullptr;
    for (uint64_t entryIndex = 0; entryIndex < memoryMapStruct->entries; ++entryIndex)
    {
        stivale2_mmap_entry memoryMapEntry = memoryMapStruct->memmap[entryIndex];
        if (memoryMapEntry.type == 1 && memoryMapEntry.length > bitmapSize)
        {
            bitmapBuffer = (uint8_t*)(memoryMapEntry.base + 0xffff'8000'0000'0000);
            break;
        }
    }

    KAssert(bitmapBuffer != nullptr, "Could not find usable memory section large enough to insert page frame bitmap.");

    pageFrameBitmap.buffer = bitmapBuffer;
    Memset(bitmapBuffer, 0xff, bitmapSize);

    for (uint64_t entryIndex = 0; entryIndex < memoryMapStruct->entries; ++entryIndex)
    {
        stivale2_mmap_entry memoryMapEntry = memoryMapStruct->memmap[entryIndex];

        if (memoryMapEntry.type == 1)
        {
            uint64_t basePageFrame = memoryMapEntry.base / 0x1000;
            for (uint64_t pageFrame = 0; pageFrame < memoryMapEntry.length / 0x1000; ++pageFrame)
            {
                pageFrameBitmap.SetBit(basePageFrame + pageFrame, false);
            }
        }
    }

    uint64_t bitmapBasePageFrame = ((uint64_t)bitmapBuffer - 0xffff'8000'0000'0000) / 0x1000;
    for (uint64_t bitmapPage = 0; bitmapPage < bitmapSize / 0x1000; ++bitmapPage)
    {
        pageFrameBitmap.SetBit(bitmapBasePageFrame + bitmapPage, true);
    }

    Serial::Print("Completed initialization of page frame allocator (physical memory allocator).");
}

void* RequestPageFrame()
{
    // Find first free page frame, starting from page frame with the lowest physical address (0)
    for (; latestAllocatedPageFrame < pageFrameBitmap.size * 8; ++latestAllocatedPageFrame)
    {
        if (!pageFrameBitmap.GetBit(latestAllocatedPageFrame))
        {
            pageFrameBitmap.SetBit(latestAllocatedPageFrame, true);
            return (void*)(latestAllocatedPageFrame * 0x1000);
        }
    }

    // Free page frame could not be found.
    Panic("Free page frame could not be found!");
    return nullptr;
}

void* RequestPageFrames(uint64_t count)
{
    uint64_t contiguousCount = 0;
    for (; latestAllocatedPageFrame < pageFrameBitmap.size * 8; ++latestAllocatedPageFrame)
    {
        if (!pageFrameBitmap.GetBit(latestAllocatedPageFrame))
        {
            contiguousCount++;
            if (contiguousCount == count)
            {
                uint64_t first = latestAllocatedPageFrame - contiguousCount + 1;
                for (uint64_t pageFrame = first; pageFrame <= latestAllocatedPageFrame; ++pageFrame)
                {
                    pageFrameBitmap.SetBit(pageFrame, true);
                }
                return (void*)(first * 0x1000);
            }
        }
        else
        {
            contiguousCount = 0;
        }
    }

    // Free page frame could not be found.
    Panic("Could not find %d contiguous free pages.", count);
    return nullptr;
}

void FreePageFrame(void* ptr)
{
    uint64_t pageFrame = (uint64_t)ptr / 0x1000;
    KAssert(pageFrameBitmap.GetBit(pageFrame), "Physical double free!");
    pageFrameBitmap.SetBit(pageFrame, false);
}

void FreePageFrames(void* ptr, uint64_t count)
{
    for (uint64_t i = 0; i < count; ++i)
    {
        FreePageFrame((void*)((uint64_t)ptr + i * 0x1000));
    }
}