#include "RAMDisk.h"
#include "Panic.h"
#include "Memory/Memory.h"

RAMDisk::RAMDisk(void* ramDiskAddr) : ramDiskVirtAddr((uint64_t)ramDiskAddr) { }

uint64_t RAMDisk::Read(uint64_t addr, void* buffer, uint64_t count, bool preAllocated)
{
    if (preAllocated)
    {
        MemCopy(buffer, (void*)(addr + ramDiskVirtAddr), count);
    }
    else
    {
        *(void**)buffer = (void*)(addr + ramDiskVirtAddr);
    }

    return count;
}

uint64_t RAMDisk::Write(uint64_t addr, const void* buffer, uint64_t count)
{
    MemCopy((void*)(addr + ramDiskVirtAddr), buffer, count);
    return count;
}