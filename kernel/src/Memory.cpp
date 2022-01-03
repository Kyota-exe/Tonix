#include "Memory.h"

void Memset(void* addr, uint8_t value, uint64_t size)
{
    uint8_t* ptr = (uint8_t*)addr;
    for (uint64_t i = 0; i < size; ++i)
    {
        ptr[i] = value;
    }
}