#pragma once

#include <stdint.h>

void InitializePageFrameAllocator();
uintptr_t RequestPageFrame();
uintptr_t RequestPageFrames(uint64_t count);
void FreePageFrame(void* ptr);
void FreePageFrames(void* ptr, uint64_t count);

extern uint64_t pageFrameCount;
