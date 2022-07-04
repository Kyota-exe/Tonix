#pragma once

#include <stdint.h>

struct AuxiliaryVector
{
    uintptr_t entry = 0;
    uintptr_t programHeaderTableAddr = 0;
    uint64_t programHeaderTableEntrySize = 0;
    uint64_t programHeaderTableEntryCount = 0;
};
