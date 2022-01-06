#ifndef MISKOS_PAGEFRAMEALLOCATOR_H
#define MISKOS_PAGEFRAMEALLOCATOR_H

#include <stdint.h>
#include "Serial.h"
#include "Stivale2Interface.h"
#include "Bitmap.h"
#include "Memory.h"

void InitializePageFrameAllocator();
void* RequestPageFrame();

extern uint64_t pageFrameCount;

#endif
