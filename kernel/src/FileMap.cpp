#include "FileMap.h"
#include "Memory/PageFrameAllocator.h"
#include "Memory/Memory.h"
#include "Scheduler.h"

enum FileMapFlag
{
    FileMapAnonymous = 0x08,
    FileMapShared = 0x02
};

void* FileMap(Task* task, void* addr, uint64_t length, int protection, int flags, int descriptor, int offset)
{
    Assert(flags & FileMapAnonymous && !(flags & FileMapShared));

    uint64_t pageCount = (length - 1) / 0x1000 + 1;

	if (addr == nullptr)
	{
		addr = task->userspaceAllocator->AllocatePages(pageCount);
	}

	Assert(reinterpret_cast<uintptr_t>(addr) % 0x1000 == 0);

    for (uint64_t page = 0; page < pageCount; ++page)
    {
        auto physAddr = reinterpret_cast<void*>(RequestPageFrame());
        auto virtAddr = reinterpret_cast<void*>((reinterpret_cast<uintptr_t>(addr) + page * 0x1000));
        task->pagingManager->MapMemory(virtAddr, physAddr, true);
    }

    Memset(addr, 0, length);

    return addr;

    (void)protection;
    (void)descriptor;
    (void)offset;
}