#ifndef MISKOS_ELFLOADER_H
#define MISKOS_ELFLOADER_H

#include <stdint.h>

uint64_t LoadELF(uint64_t ramDiskBegin, PagingManager* pagingManager);

#endif
