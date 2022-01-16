#include "Memory/PageFrameAllocator.h"
#include "Memory/PagingManager.h"
#include "Stivale2Interface.h"
#include "IDT.h"
#include "PIC.h"
#include "Vector.h"
#include "Ext2.h"
#include "Serial.h"
#include "SMP.h"
#include "GDT.h"
#include "TSS.h"
#include "Task.h"
#include "Scheduler.h"

PagingManager kernelPagingManager;

extern "C" void _start(stivale2_struct* stivale2Struct)
{
    InitializeStivale2Interface(stivale2Struct);

    Serial::Print("\nKernel ELF successfully loaded", "\n\n");

    InitializeGDT();
    LoadGDT();
    InitializeIDT();
    LoadIDT();
    InitializePageFrameAllocator();
    kernelPagingManager.InitializePaging();
    kernelPagingManager.SetCR3();
    InitializeKernelHeap();
    InitializeTSS();
    InitializePIC();
    ActivatePICKeyboardInterrupts();
    asm volatile("sti");

    InitializeTaskList();

    auto modulesStruct = (stivale2_struct_tag_modules*)GetStivale2Tag(STIVALE2_STRUCT_TAG_MODULES_ID);
    Serial::Printf("Module Count: %d", modulesStruct->module_count);
    for (uint64_t i = 0; i < modulesStruct->module_count; ++i)
    {
        stivale2_module module = modulesStruct->modules[i];

        if (StringEquals(module.string, "boot:///ext2-ramdisk-image.ext2"))
        {
            //InitializeExt2(module.begin, module.end);
        }
        else if (StringEquals(module.string, "boot:///proc.elf"))
        {
            CreateProcess(module.begin, 'O');
            CreateProcess(module.begin, 'X');
            CreateProcess(module.begin, 'I');
        }
    }

    //StartSchedulerOnNonBSPCores();
    StartScheduler();

    while (true) asm("hlt");
}