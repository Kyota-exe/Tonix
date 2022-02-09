#include "Memory/Memory.h"

void Memset(void* addr, uint8_t value, uint64_t size)
{
    auto ptr = (uint8_t*)addr;
    for (uint64_t i = 0; i < size; ++i)
    {
        ptr[i] = value;
    }
}

void MemCopy(void* destination, const void* source, uint64_t count)
{
    auto src = (uint8_t*)source;
    auto dest = (uint8_t*)destination;

    for (uint64_t i = 0; i < count; ++i)
    {
        dest[i] = src[i];
    }
}

uintptr_t HigherHalf(uintptr_t physAddr)
{
	return physAddr + 0xffff'8000'0000'0000;
}