#ifndef MISKOS_HEAP_H
#define MISKOS_HEAP_H

#include <stdint.h>

void InitializeKernelHeap();
void* KMalloc(uint64_t size);
void KFree(void* ptr);

#endif
