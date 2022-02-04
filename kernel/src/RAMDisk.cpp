#include "RAMDisk.h"
#include "Memory/Memory.h"

RAMDisk::RAMDisk(void* ramDiskAddr) : ramDiskVirtAddr((uint64_t)ramDiskAddr) { }

void RAMDisk::Read(uint64_t addr, void* buffer, uint64_t count)
{
    MemCopy(buffer, (void*)(addr + ramDiskVirtAddr), count);
}

void RAMDisk::AllocateRead(uint64_t addr, void** bufferPtr, uint64_t count)
{
    *bufferPtr = (void*)(addr + ramDiskVirtAddr);

    (void)count;
}

void RAMDisk::Write(uint64_t addr, const void* buffer, uint64_t count)
{
    MemCopy((void*)(addr + ramDiskVirtAddr), buffer, count);
}