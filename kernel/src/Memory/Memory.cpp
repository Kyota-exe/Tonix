#include "Memory/Memory.h"

void* memset(void* ptr, uint8_t value, uint64_t num)
{
    auto p = reinterpret_cast<uint8_t*>(ptr);
    for (uint64_t i = 0; i < num; ++i)
    {
        p[i] = value;
    }

    return ptr;
}

void* memcpy(void* destination, const void* source, uint64_t num)
{
    auto src = reinterpret_cast<const uint8_t*>(source);
    auto dest = reinterpret_cast<uint8_t*>(destination);

    for (uint64_t i = 0; i < num; ++i)
    {
        dest[i] = src[i];
    }

    return destination;
}

uintptr_t HigherHalf(uintptr_t physAddr)
{
	return physAddr + 0xffff'8000'0000'0000;
}

bool memcmp(const void* left, const void* right, uint64_t count)
{
	auto x = static_cast<const uint8_t*>(left);
	auto y = static_cast<const uint8_t*>(right);

	for (uint64_t i = 0; i < count; ++i)
	{
		if (x[i] != y[i]) return false;
	}

	return true;
}