#include "FileMap.h"
#include "Memory/PageFrameAllocator.h"
#include "Memory/Memory.h"
#include "Scheduler.h"

void* FileMap(void* addr, uint64_t length)
{
    Assert(length > 0);

    Task& task = Scheduler::GetScheduler()->currentTask;
    uint64_t pageCount = (length - 1) / 0x1000 + 1;

	if (addr == nullptr)
	{
		addr = task.userspaceAllocator->AllocatePages(pageCount);
	}

	Assert(reinterpret_cast<uintptr_t>(addr) % 0x1000 == 0);

    for (uint64_t pageIndex = 0; pageIndex < pageCount; ++pageIndex)
    {
        auto physAddr = reinterpret_cast<void*>(RequestPageFrame());
        auto virtAddr = reinterpret_cast<void*>((reinterpret_cast<uintptr_t>(addr) + pageIndex * 0x1000));
        task.pagingManager->MapMemory(virtAddr, physAddr, true);
    }

    Memset(addr, 0, length);

    return addr;
}