#pragma once

#include <stdint.h>
#include "Process.h"

class ELFLoader
{
public:
    static uint64_t LoadELF(const String& path, Process* process);
};
