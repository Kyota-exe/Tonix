#pragma once

#include "Scheduler.h"

struct CPU
{
    Scheduler* scheduler;
    static uint32_t GetCoreID();
    static void EnableSSE();
};