#pragma once

#include <stdint.h>

class ELFLoader
{
public:
    static uint64_t LoadELF(uint64_t ramDiskBegin, PagingManager* pagingManager);
};
