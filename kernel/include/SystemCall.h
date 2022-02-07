#pragma once

#include <stdint.h>
#include "Error.h"

enum class SystemCallType
{
    Open = 0,
    Read = 1,
    Write = 2,
    Seek = 3,
    Close = 4,
    FileMap = 5,
    Panic = 254,
    Log = 255,
    Special = 69
};

uint64_t SystemCall(SystemCallType type, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, Error& error);