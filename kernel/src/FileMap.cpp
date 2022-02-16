#include "FileMap.h"
#include "Panic.h"
#include "Memory/PageFrameAllocator.h"
#include "Memory/Memory.h"
#include "Scheduler.h"

enum FileMapFlag
{
    FileMapAnonymous = 0x08,
    FileMapShared = 0x02
};

void* FileMap(void* addr, uint64_t length, int protection, int flags, int descriptor, int offset)
{
    KAssert(flags & FileMapAnonymous, "File-backed mmap is not supported.");
    KAssert(!(flags & FileMapShared), "Shared mmap is not supported");

	Process process = taskList->Get(currentTaskIndex);

    uint64_t pageCount = (length - 1) / 0x1000 + 1;

	if (addr == nullptr)
	{
		addr = process.userspaceAllocator->AllocatePages(pageCount);
	}

    for (uint64_t page = 0; page < pageCount; ++page)
    {
        void* physAddr = RequestPageFrame();
        void* virtAddr = (void*)((uint64_t)addr + page * 0x1000);
        process.pagingManager->MapMemory(virtAddr, physAddr, true);
    }

    Memset(addr, 0, length);

    return addr;

    (void)protection;
    (void)descriptor;
    (void)offset;
}