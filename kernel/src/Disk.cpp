#include "Disk.h"

void Disk::AllocateRead(uint64_t addr, void** bufferPtr, uint64_t count)
{
    *bufferPtr = new char[count];
    Read(addr, *bufferPtr, count);
}