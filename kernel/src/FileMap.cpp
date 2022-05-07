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
        auto physAddr = RequestPageFrame();
        Memset(reinterpret_cast<void*>(HigherHalf(physAddr)), 0, 0x1000);
        auto virtAddr = reinterpret_cast<void*>((reinterpret_cast<uintptr_t>(addr) + pageIndex * 0x1000));
        task.pagingManager->MapMemory(virtAddr, reinterpret_cast<void*>(physAddr), true);
    }

    return addr;
}