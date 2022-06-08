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
    TCBSet = 6,
    Clock = 7,
    Exit = 8,
    Sleep = 9,
    Stat = 10,
    FStat = 11,
    SetTerminalSettings = 12,
    GetTerminalWindowSize = 13,
    GetPID = 14,
    GetWorkingDirectory = 15,
    Fork = 16,
    Wait = 17,
    GetParentPID = 18,
    SetWorkingDirectory = 19,
    Execute = 20,
    ReadDirectory = 21,
    GetFileDescriptorFlags = 22,
    Panic = 254,
    Log = 255
};

struct InterruptFrame;

uint64_t SystemCall(SystemCallType type, uint64_t arg0, uint64_t arg1, uint64_t arg2, InterruptFrame* interruptFrame, Error& error);
