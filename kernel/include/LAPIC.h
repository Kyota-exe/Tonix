#pragma once

#include <stdint.h>

class LAPIC
{
public:
    void SetTimeBetweenTimerFires(uint64_t milliseconds);
    uint64_t GetTimeRemainingMilliseconds() const;
    void SendEOI();
    LAPIC();
private:
    uint64_t apicRegisterBase;
    uint64_t lapicTimerBaseFrequency;
    uint64_t GetTimerBaseFrequency();
    uint64_t GetBaseMSR();
};
