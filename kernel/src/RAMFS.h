#ifndef MISKOS_RAMFS_H
#define MISKOS_RAMFS_H

#include <stdint.h>
#include "Heap.h"
#include "Serial.h"

void InitializeRAMFS(uint64_t ramDiskBegin, uint64_t ramDiskEnd);

#endif
