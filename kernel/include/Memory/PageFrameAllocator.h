#pragma once

#include <stdint.h>

void InitializePageFrameAllocator();
void* RequestPageFrame();

extern uint64_t pageFrameCount;
