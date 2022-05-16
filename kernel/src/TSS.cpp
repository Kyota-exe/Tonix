#include "TSS.h"
#include "Memory/Memory.h"
#include "Memory/PageFrameAllocator.h"

TSS* TSS::Initialize()
{
    auto tss = new TSS;
    Memset(tss, 0, sizeof(TSS));

    tss->rsp0 = HigherHalf(RequestPageFrame() + 0x1000);
    tss->ist1 = HigherHalf(RequestPageFrame() + 0x1000); // Double Fault
    tss->ist2 = HigherHalf(RequestPageFrame() + 0x1000); // Non-Maskable Interrupt
    tss->ist3 = HigherHalf(RequestPageFrame() + 0x1000); // Machine Check
    tss->ist4 = HigherHalf(RequestPageFrame() + 0x1000); // Debug
    tss->ist5 = HigherHalf(RequestPageFrame() + 0x1000); // Timer Interrupt
    tss->ioMapOffset = sizeof(TSS);

    return tss;
}

void TSS::SetSystemCallStack(void* syscallStack)
{
    ist7 = reinterpret_cast<uintptr_t>(syscallStack);
}