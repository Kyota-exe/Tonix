#pragma once

#include <stdint.h>

struct TimerEntry
{
    uint64_t milliseconds;

    bool unblockOnExpire = false;
    uint64_t pid = 0;
};
