#ifndef MISKOS_HEAP_H
#define MISKOS_HEAP_H

#include <stdint.h>
#include "Serial.h"
#include "PageFrameAllocator.h"

void* KMalloc(uint64_t size);
void KFree(void* ptr);

#endif
