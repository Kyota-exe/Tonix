#pragma once

#include <stdint.h>

void* memset(void* ptr, uint8_t value, uint64_t num);
void* memcpy(void* destination, const void* source, uint64_t num);
uintptr_t HigherHalf(uintptr_t physAddr);