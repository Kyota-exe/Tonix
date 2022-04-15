#pragma once

#include "Scheduler.h"

struct CPU
{
    Scheduler* scheduler;
    TSS* tss;
    static uint32_t GetCoreID();
    static void EnableSSE();
};