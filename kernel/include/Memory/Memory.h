#pragma once

#include <stdint.h>

void Memset(void* addr, uint8_t value, uint64_t size);
void MemCopy(void* destination, const void* source, uint64_t count);
bool MemCompare(const void* left, const void* right, uint64_t count);
uintptr_t HigherHalf(uintptr_t physAddr);