#include "FileMap.h"
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
    Assert(flags & FileMapAnonymous && !(flags & FileMapShared));

	Task process = taskList->Get(currentTaskIndex);

    uint64_t pageCount = (length - 1) / 0x1000 + 1;

	if (addr == nullptr)
	{
		addr = process.userspaceAllocator->AllocatePages(pageCount);
	}

	Assert(reinterpret_cast<uintptr_t>(addr) % 0x1000 == 0);

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