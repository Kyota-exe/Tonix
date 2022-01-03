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

    Serial::Printf("%x", *(uint8_t*)0x981723);
    *(uint8_t*)0x981723 = 0xff;
    Serial::Printf("%x", *(uint8_t*)(0x981723 + 0xffff'8000'0000'0000));

    while (true) asm("hlt");
}