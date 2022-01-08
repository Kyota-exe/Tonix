#include "Memory.h"

void Memset(void* addr, uint8_t value, uint64_t size)
{
    uint8_t* ptr = (uint8_t*)addr;
    for (uint64_t i = 0; i < size; ++i)
    {
        ptr[i] = value;
    }
}

void MemCopy(void* destination, void* source, uint64_t count)
{
    uint8_t* src = (uint8_t*)source;
    uint8_t* dest = (uint8_t*)destination;

    for (uint64_t i = 0; i < count; ++i)
    {
        dest[i] = src[i];
    }
}