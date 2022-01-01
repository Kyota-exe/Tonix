#ifndef MISKOS_PAGEFRAMEALLOCATOR_H
#define MISKOS_PAGEFRAMEALLOCATOR_H

#include <stdint.h>
#include "Serial.h"
#include "Stivale2Interface.h"
#include "Bitmap.h"

void InitializePageFrameAllocator(stivale2_struct *stivale2Struct);
void RequestPage();

#endif
