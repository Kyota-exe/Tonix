#ifndef MISKOS_PAGEFRAMEALLOCATOR_H
#define MISKOS_PAGEFRAMEALLOCATOR_H

#include <stdint.h>

void InitializePageFrameAllocator();
void* RequestPageFrame();

extern uint64_t pageFrameCount;

#endif
