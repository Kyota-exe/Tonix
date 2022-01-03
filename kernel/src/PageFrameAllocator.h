#ifndef MISKOS_PAGEFRAMEALLOCATOR_H
#define MISKOS_PAGEFRAMEALLOCATOR_H

#include <stdint.h>
#include "Serial.h"
#include "Stivale2Interface.h"
#include "Bitmap.h"
#include "Memory.h"

namespace PageFrameAllocator
{
    void InitializePageFrameAllocator(stivale2_struct *stivale2Struct);
    void* RequestPageFrame();

    extern bool initialized;
    extern uint64_t pageFrameCount;
}

#endif
