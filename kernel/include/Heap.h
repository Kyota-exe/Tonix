#pragma once

#include <stdint.h>

enum class Allocator
{
    Slab, Permanent
};

void InitializeKernelHeap();

void* operator new(uint64_t, void* ptr);
void* operator new[](uint64_t, void* ptr);
void* operator new(uint64_t size);
void* operator new[](uint64_t size);
void operator delete(void* ptr);
void operator delete(void* ptr, uint64_t);
void operator delete[](void* ptr);
void* operator new(uint64_t size, Allocator type);
void* operator new[](uint64_t size, Allocator type);