#include "LAPIC.h"
#include "PIT.h"
#include "Memory/Memory.h"
#include "Assert.h"

constexpr uint64_t APIC_EIO_OFFSET = 0xb0;
constexpr uint64_t APIC_SPURIOUS_INTERRUPT_VECTOR = 0xf0;
constexpr uint64_t APIC_DIVIDE_CONFIG = 0x3e0;
constexpr uint64_t APIC_LVT_TIMER = 0x320;
constexpr uint64_t APIC_INITIAL_COUNT = 0x380;
constexpr uint64_t APIC_CURRENT_COUNT = 0x390;

void LAPIC::SendEOI()
{
    *(volatile uint32_t*)(apicRegisterBase + APIC_EIO_OFFSET) = 0;
}

LAPIC::LAPIC()
{
    apicRegisterBase = HigherHalf((GetBaseMSR() & ~0xfff));

    // Activate LAPIC and map the Spurious Interrupt Vector to interrupt gate 255
    *(volatile uint32_t*)(apicRegisterBase + APIC_SPURIOUS_INTERRUPT_VECTOR) = 0x1ff;

    // Divide by 2
    *(volatile uint32_t*)(apicRegisterBase + APIC_DIVIDE_CONFIG) = 0;

    // Interrupt gate 48, one-shot
    *(volatile uint32_t*)(apicRegisterBase + APIC_LVT_TIMER) = 0b00'0'000'0'0000'00110000;

    lapicTimerBaseFrequency = GetTimerBaseFrequency();

    *(volatile uint32_t*)(apicRegisterBase + APIC_INITIAL_COUNT) = 0;
}

void LAPIC::SetTimeBetweenTimerFires(uint64_t milliseconds)
{
    uint64_t count = lapicTimerBaseFrequency * milliseconds / 1000;

    Assert(count <= UINT32_MAX);
    *(volatile uint32_t*)(apicRegisterBase + APIC_INITIAL_COUNT) = static_cast<uint32_t>(count);
}

uint64_t LAPIC::GetTimeRemainingMilliseconds() const
{
    uint64_t count = *(volatile uint32_t*)(apicRegisterBase + APIC_CURRENT_COUNT);
    return count * 1000 / lapicTimerBaseFrequency;
}

uint64_t LAPIC::GetTimerBaseFrequency()
{
    // Initialize variables and pointers here to make instructions as fast as possible while calibrating.
    uint64_t lapicTicksCount = 0xfffff;
    auto currentCountRegister = (volatile uint32_t*)(apicRegisterBase + APIC_CURRENT_COUNT);
    auto initialCountRegister = (volatile uint32_t*)(apicRegisterBase + APIC_INITIAL_COUNT);

    // Set the PIT's reload value to its max value so that the PIT takes a long time to reset the
    // current tick value to 0, so that the LAPIC timer has enough time to tick {lapicTicksCount} times.
    PITSetReloadValue(UINT16_MAX);

    uint16_t initialPITTick = PITGetTick();

    // Set the number of samples
    *initialCountRegister = lapicTicksCount;

    // Wait until the LAPIC finishes counting
    while (*currentCountRegister != 0) continue;

    uint16_t endPITTick = PITGetTick();

    // Stop the LAPIC timer
    *initialCountRegister = 0;

    return (lapicTicksCount / (initialPITTick - endPITTick)) * PIT_BASE_FREQUENCY;
}

uint64_t LAPIC::GetBaseMSR()
{
    uint32_t low;
    uint32_t high;
    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(0x1b));
    return (uint64_t)high << 32 | low;
}