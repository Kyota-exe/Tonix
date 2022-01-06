#include "KernelUtilities.h"

PagingManager kernelPagingManager;

extern "C" void _start(stivale2_struct *stivale2Struct)
{
    InitializeStivale2Interface(stivale2Struct);

    Serial::Print("\nKernel ELF successfully loaded", "\n\n");

    LoadIDT();
    PageFrameAllocator::InitializePageFrameAllocator(stivale2Struct);
    kernelPagingManager.InitializePaging(stivale2Struct);
    ActivateLAPIC();
    //ActivateLAPICTimer();
    InitializePIC();
    ActivatePICKeyboardInterrupts();
    asm volatile("sti");

    while (true) asm("hlt");
}