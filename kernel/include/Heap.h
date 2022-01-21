#ifndef MISKOS_HEAP_H
#define MISKOS_HEAP_H

#include <stdint.h>

void InitializeKernelHeap();

void* operator new(uint64_t, void* ptr);
void* operator new[](uint64_t, void* ptr);
void* operator new(uint64_t size);
void* operator new[](uint64_t size);
void operator delete(void* ptr);
void operator delete(void* ptr, uint64_t);
void operator delete[](void* ptr);

#endif
