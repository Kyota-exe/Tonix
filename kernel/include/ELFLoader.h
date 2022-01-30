#pragma once

#include <stdint.h>

uint64_t LoadELF(uint64_t ramDiskBegin, PagingManager* pagingManager);
