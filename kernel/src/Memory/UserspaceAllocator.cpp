#include "UserspaceAllocator.h"

constexpr uintptr_t BUMP_ALLOCATOR_BASE = 0x0001'0000'0000;

UserspaceAllocator::UserspaceAllocator() : currentAddr(BUMP_ALLOCATOR_BASE) {}

void* UserspaceAllocator::AllocatePages(uint64_t pageCount)
{
	void* addr = reinterpret_cast<void*>(currentAddr);
	currentAddr += pageCount * 0x1000;
	return addr;
}