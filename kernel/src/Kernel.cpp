#include "KernelUtilities.h"

PagingManager kernelPagingManager;

extern "C" void _start(stivale2_struct *stivale2Struct)
{
    InitializeStivale2Interface(stivale2Struct);

    Serial::Print("\nKernel ELF successfully loaded", "\n\n");

    LoadIDT();
    InitializePageFrameAllocator();
    kernelPagingManager.InitializePaging();
    ActivateLAPIC();
    //ActivateLAPICTimer();
    InitializePIC();
    ActivatePICKeyboardInterrupts();
    asm volatile("sti");

    while (true) asm("hlt");
}