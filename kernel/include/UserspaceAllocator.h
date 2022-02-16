#pragma once

#include <stdint.h>

class UserspaceAllocator
{
public:
	void* AllocatePages(uint64_t pageCount);
	UserspaceAllocator();
private:
	uintptr_t currentAddr;
};