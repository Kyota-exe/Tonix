#pragma once

#include <stdint.h>

class LAPIC
{
public:
    enum class TimerMode : uint8_t
    {
        OneShot = 0,
        Periodic = 1,
        TSCDeadline = 2,
    };

    static void Activate();
    static void SetTimerMode(TimerMode mode);
    //static void SetTimerFrequency(uint64_t frequency);
    static void SetTimeBetweenTimerFires(uint64_t milliseconds);
    static void SetTimerMask(bool mask);
    static void SendEOI();
    static void CalibrateTimer();
private:
    static uint64_t GetBaseMSR();
};
