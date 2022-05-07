#pragma once

#include "Scheduler.h"

struct CPU
{
    Scheduler* scheduler = nullptr;
    static uint32_t GetCoreID();
    static CPU GetStruct();
    static void InitializeCPUList(unsigned long cpuCount);
    static void InitializeCPUStruct(Scheduler* scheduler);
    static void EnableSSE();
};