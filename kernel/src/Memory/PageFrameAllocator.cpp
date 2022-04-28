#include "Memory/Memory.h"
#include "Memory/PageFrameAllocator.h"
#include "Serial.h"
#include "Stivale2Interface.h"
#include "Assert.h"
#include "Bitmap.h"
#include "Spinlock.h"
#include "Heap.h"

Bitmap pageFrameBitmap = Bitmap(nullptr, 0, false);
Spinlock pageFrameBitmapLock;

uint64_t pageFrameCount = 0;
uint64_t latestAllocatedPageFrame = 0;

void InitializePageFrameAllocator()
{
    auto memoryMapStruct = (stivale2_struct_tag_memmap*)GetStivale2Tag(STIVALE2_STRUCT_TAG_MEMMAP_ID);
    stivale2_mmap_entry lastMemoryMapEntry = memoryMapStruct->memmap[memoryMapStruct->entries - 1];

    uint64_t memorySize = lastMemoryMapEntry.base + lastMemoryMapEntry.length;
    pageFrameCount = memorySize / 0x1000;
    uint64_t bitmapSize = pageFrameCount / 8;

    uint8_t* bitmapBuffer = nullptr;
    for (uint64_t entryIndex = 0; entryIndex < memoryMapStruct->entries; ++entryIndex)
    {
        stivale2_mmap_entry memoryMapEntry = memoryMapStruct->memmap[entryIndex];
        if (memoryMapEntry.type == 1 && memoryMapEntry.length > bitmapSize)
        {
            bitmapBuffer = reinterpret_cast<uint8_t*>(HigherHalf(memoryMapEntry.base));
            break;
        }
    }

    Assert(bitmapBuffer != nullptr);

    Memset(bitmapBuffer, 0xff, bitmapSize);
    new (&pageFrameBitmap) Bitmap(bitmapBuffer, bitmapSize, false);

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
}

uintptr_t RequestPageFrame()
{
    pageFrameBitmapLock.Acquire();

    // Find first free page frame, starting from page frame with the lowest physical address (0)
    for (; latestAllocatedPageFrame < pageFrameBitmap.TotalNumberOfBits(); ++latestAllocatedPageFrame)
    {
        if (!pageFrameBitmap.GetBit(latestAllocatedPageFrame))
        {
            pageFrameBitmap.SetBit(latestAllocatedPageFrame, true);
            pageFrameBitmapLock.Release();
            return latestAllocatedPageFrame * 0x1000;
        }
    }

    pageFrameBitmapLock.Release();
    Serial::Log("Failed to find free page frame.");
    Panic();
}

uintptr_t RequestPageFrames(uint64_t count)
{
    pageFrameBitmapLock.Acquire();

    uint64_t contiguousCount = 0;
    for (; latestAllocatedPageFrame < pageFrameBitmap.TotalNumberOfBits(); ++latestAllocatedPageFrame)
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
                pageFrameBitmapLock.Release();
                return first * 0x1000;
            }
        }
        else
        {
            contiguousCount = 0;
        }
    }

    pageFrameBitmapLock.Release();
    Serial::Log("Could not find %d continuous free pages.", count);
    Panic();
}

void FreePageFrame(void* ptr)
{
    uint64_t pageFrame = (uint64_t)ptr / 0x1000;

    pageFrameBitmapLock.Acquire();

    Assert(pageFrameBitmap.GetBit(pageFrame));
    pageFrameBitmap.SetBit(pageFrame, false);

    pageFrameBitmapLock.Release();
}

void FreePageFrames(void* ptr, uint64_t count)
{
    for (uint64_t i = 0; i < count; ++i)
    {
        FreePageFrame((void*)((uint64_t)ptr + i * 0x1000));
    }
}