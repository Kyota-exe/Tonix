#ifndef MISKOS_MEMORY_H
#define MISKOS_MEMORY_H

#include <stdint.h>

void Memset(void* addr, uint8_t value, uint64_t size);
void MemCopy(void* destination, void* source, uint64_t count);

#endif
