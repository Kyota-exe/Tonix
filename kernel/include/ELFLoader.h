#pragma once

#include <stdint.h>

class ELFLoader
{
public:
    static uint64_t LoadELF(int elfFile, PagingManager* pagingManager);
};
