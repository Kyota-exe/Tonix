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
    Serial::Printf("Local APIC timer frequency: %d Hz", lapicTimerFreq, "\n\n");

    stivale2_struct_tag_modules* modulesStruct = (stivale2_struct_tag_modules*)GetStivale2Tag(STIVALE2_STRUCT_TAG_MODULES_ID);
    Serial::Printf("Module Count: %d", modulesStruct->module_count);
    for (uint64_t i = 0; i < modulesStruct->module_count; ++i)
    {
        stivale2_module module = modulesStruct->modules[i];
        if (StringEquals(module.string, "boot:///initrd.tar"))
        {
            Serial::Print("Initializing RAM filesystem...");
            InitializeRAMFS(module.begin, module.end);
        }
    }

    //StartNonBSPCores();

    while (true) asm("hlt");
}