#include "APIC.h"

static uint64_t apicRegisterBase = 0;

static uint64_t GetLAPICBaseMSR()
{
    uint32_t low;
    uint32_t high;
    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(0x1b));
    return (uint64_t)high << 32 | low;
}

void LAPICEOI()
{
    *(volatile uint32_t*)(apicRegisterBase + 0xb0) = 0;
}

void ActivateLAPIC()
{
    Serial::Print("Activating the local APIC...");
    apicRegisterBase = (GetLAPICBaseMSR() & ~0xfff) + 0xffff'8000'0000'0000;
    Serial::Printf("Local APIC Register Base: %x", apicRegisterBase);

    // Activate APIC and set the Spurious Interrupt Vector to map to 255 in the IDT
    *(volatile uint32_t*)(apicRegisterBase + 0xf0) = 0x1ff;

    asm volatile("sti");
    Serial::Print("Activated the local APIC.", "\n\n");
}

void ActivateLAPICTimer()
{
    // Set LVT timer register for the local APIC timer
    // Set the local APIC timer to map to interrupt gate 48 in the IDT
    *(volatile uint32_t*)(apicRegisterBase + 0x320) = 0b01'0'000'0'0000'00110000;

    // Set divide configuration register for the local APIC timer
    *(volatile uint32_t*)(apicRegisterBase + 0x3e0) = 0b1'0'11;

    // Set initial count register for the local APIC timer
    *(volatile uint32_t*)(apicRegisterBase + 0x380) = 10000000;
}