#include "Stivale2Interface.h"
#include "IDT.h"
#include "PageFrameAllocator.h"
#include "PagingManager.h"
#include "Serial.h"

extern "C" void _start(stivale2_struct *stivale2Struct)
{
    InitializeStivale2Interface(stivale2Struct);

    Serial::Print("\nKernel ELF successfully loaded", "\n\n");

    LoadIDT();
    PageFrameAllocator::InitializePageFrameAllocator(stivale2Struct);

    PagingManager pagingManager;
    pagingManager.InitializePaging(stivale2Struct);

    while (true) asm("hlt");
}