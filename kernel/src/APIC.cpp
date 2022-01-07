#include "APIC.h"

const uint64_t APIC_EIO_OFFSET = 0xb0;
const uint64_t APIC_SPURIOUS_INTERRUPT_VECTOR = 0xf0;
const uint64_t APIC_DIVIDE_CONFIG = 0x3e0;
const uint64_t APIC_LVT_TIMER = 0x320;
const uint64_t APIC_INITIAL_COUNT = 0x380;
const uint64_t APIC_CURRENT_COUNT = 0x390;

static uint64_t apicRegisterBase = 0;

static uint64_t GetLAPICBaseMSR()
{
    uint32_t low;
    uint32_t high;
    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(0x1b));
    return (uint64_t)high << 32 | low;
}

void LAPICSendEOI()
{
    *(volatile uint32_t*)(apicRegisterBase + APIC_EIO_OFFSET) = 0;
}

void ActivateLAPIC()
{
    apicRegisterBase = (GetLAPICBaseMSR() & ~0xfff) + 0xffff'8000'0000'0000;

    // Activate APIC and set the Spurious Interrupt Vector to map to 255 in the IDT
    *(volatile uint32_t*)(apicRegisterBase + APIC_SPURIOUS_INTERRUPT_VECTOR) = 0x1ff;
}

uint64_t CalibrateLAPICTimer()
{
    // Divide by 2
    *(volatile uint32_t*)(apicRegisterBase + APIC_DIVIDE_CONFIG) = 0;

    // Initialize variables and pointers here to make instructions as fast as possible while calibrating.
    uint64_t lapicTicksCount = 0xfffff;
    volatile uint32_t* currentCountRegister = (volatile uint32_t*)(apicRegisterBase + APIC_CURRENT_COUNT);
    volatile uint32_t* initialCountRegister = (volatile uint32_t*)(apicRegisterBase + APIC_INITIAL_COUNT);

    // Interrupt gate 48 in the IDT, masked, one-shot
    *(volatile uint32_t*)(apicRegisterBase + APIC_LVT_TIMER) = 0b00'1'000'0'0000'00110000;

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