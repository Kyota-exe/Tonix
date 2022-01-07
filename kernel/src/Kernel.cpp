#include "KernelUtilities.h"

PagingManager kernelPagingManager;

extern "C" void _start(stivale2_struct* stivale2Struct)
{
    InitializeStivale2Interface(stivale2Struct);

    Serial::Print("\nKernel ELF successfully loaded", "\n\n");

    LoadIDT();
    InitializePageFrameAllocator();
    kernelPagingManager.InitializePaging();
    InitializePIC();
    ActivateLAPIC();
    ActivatePICKeyboardInterrupts();
    asm volatile("sti");

    uint64_t lapicTimerFreq = CalibrateLAPICTimer();
    Serial::Printf("Local APIC timer frequency: %d Hz", lapicTimerFreq);

    StartNonBSPCores();

    while (true) asm("hlt");
}