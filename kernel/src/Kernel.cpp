#include "Memory/PageFrameAllocator.h"
#include "Memory/PagingManager.h"
#include "Stivale2Interface.h"
#include "IDT.h"
#include "PIC.h"
#include "Ext2.h"
#include "Serial.h"
#include "Heap.h"
#include "GDT.h"
#include "TSS.h"
#include "Task.h"
#include "Scheduler.h"

PagingManager kernelPagingManager;

extern "C" void _start(stivale2_struct* stivale2Struct)
{
    InitializeStivale2Interface(stivale2Struct);

    Serial::Print("Kernel ELF successfully loaded");

    InitializeGDT();
    LoadGDT();
    InitializeIDT();
    LoadIDT();
    // TODO: Does the "pre-init-process" kernel even need custom paging? Seems like a lot of wasted page frames to me.
    InitializePageFrameAllocator();
    kernelPagingManager.InitializePaging();
    kernelPagingManager.SetCR3();
    InitializeKernelHeap();
    InitializeTSS();
    InitializePIC();
    ActivatePICKeyboardInterrupts();
    InitializeTaskList();

    auto modulesStruct = (stivale2_struct_tag_modules*)GetStivale2Tag(STIVALE2_STRUCT_TAG_MODULES_ID);
    Serial::Printf("Module Count: %d", modulesStruct->module_count);
    for (uint64_t i = 0; i < modulesStruct->module_count; ++i)
    {
        stivale2_module module = modulesStruct->modules[i];

        if (String::Equals(module.string, "boot:///ext2-ramdisk-image.ext2"))
        {
            Ext2::Initialize(module.begin, module.end);
        }
        else if (String::Equals(module.string, "boot:///proc.elf"))
        {
            CreateProcess(module.begin, 'O');
            //CreateProcess(module.begin, 'X');
            //CreateProcess(module.begin, 'I');
        }
    }

    //StartSchedulerOnNonBSPCores();
    //StartScheduler();

    while (true) asm("hlt");
}