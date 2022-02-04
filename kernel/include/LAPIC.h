#pragma once

#include <stdint.h>

class LAPIC
{
public:
    static void Activate();
    static void SetTimerMode(uint8_t mode);
    static void SetTimerFrequency(uint64_t frequency);
    static void SetTimerMask(bool mask);
    static void SendEOI();
    static void CalibrateTimer();
private:
    static uint64_t GetBaseMSR();
};
