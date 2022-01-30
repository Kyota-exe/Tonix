#pragma once

#include <stdint.h>

void Memset(void* addr, uint8_t value, uint64_t size);
void MemCopy(void* destination, const void* source, uint64_t count);