#include "Memory/PageFrameAllocator.h"
#include "Memory/PagingManager.h"
#include "Stivale2Interface.h"
#include "IDT.h"
#include "PIC.h"
#include "Ext2.h"
#include "Serial.h"
#include "Heap.h"
#include "GDT.h"
#include "Scheduler.h"
#include "Device.h"
#include "Framebuffer.h"
#include "TextRenderer.h"

PagingManager kernelPagingManager;

extern "C" void _start(stivale2_struct* stivale2Struct)
{
    InitializeStivale2Interface(stivale2Struct);

    Serial::Print("Kernel ELF successfully loaded");

    InitializePageFrameAllocator();
    kernelPagingManager.InitializePaging();
    kernelPagingManager.SetCR3();
    InitializeKernelHeap();

    GDT::Initialize();
    GDT::InitializeTSS();
    GDT::LoadGDTR();
    GDT::LoadTSS();
    InitializeIDT();
    LoadIDT();

    InitializePIC();
    ActivatePICKeyboardInterrupts();
    InitializeTaskList();
    Framebuffer::Initialize();

    auto modulesStruct = (stivale2_struct_tag_modules*)GetStivale2Tag(STIVALE2_STRUCT_TAG_MODULES_ID);
    Serial::Printf("Module Count: %d", modulesStruct->module_count);
    for (uint64_t i = 0; i < modulesStruct->module_count; ++i)
    {
        stivale2_module module = modulesStruct->modules[i];

        if (String(module.string).Equals("boot:///ext2-ramdisk-image.ext2"))
        {
            InitializeVFS((void*)module.begin);
        }
        else if (String(module.string).Equals("boot:///proc.elf"))
        {
            CreateProcess(module.begin, 'O');
            //CreateProcess(module.begin, 'X');
            //CreateProcess(module.begin, 'I');
        }
    }

    //StartSchedulerOnNonBSPCores();
    StartScheduler();

    while (true) asm("hlt");
}

extern "C" __attribute__((unused)) void __cxa_pure_virtual()
{
    Panic("__cxa_pure_virtual called");
}