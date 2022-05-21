#include "Memory/PageFrameAllocator.h"
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

constexpr const char* SHELL_PATH = "/usr/bin/bash";

extern "C" void _start(stivale2_struct* stivale2Struct)
{
    InitializeStivale2Interface(stivale2Struct);

    Serial::Log("Kernel ELF successfully loaded");

    InitializePageFrameAllocator();
    InitializeKernelHeap();
    PagingManager::SaveBootloaderAddressSpace();

    GDT::Initialize();
    TSS* tss = TSS::Initialize();
    GDT::LoadGDTR();
    GDT::LoadTSS(tss);
    IDT::Initialize();
    IDT::Load();

    InitializePIC();
    ActivatePICKeyboardInterrupts();
    Framebuffer::Initialize();

    auto modulesStruct = (stivale2_struct_tag_modules*)GetStivale2Tag(STIVALE2_STRUCT_TAG_MODULES_ID);
    Serial::Log("Module Count: %d", modulesStruct->module_count);
    for (uint64_t i = 0; i < modulesStruct->module_count; ++i)
    {
        stivale2_module module = modulesStruct->modules[i];

        if (String(module.string).Equals("boot:///ext2-ramdisk-image.ext2"))
        {
            VFS::Initialize((void*)module.begin);
        }
    }

    Scheduler::InitializeQueue();

    {
        Vector<String> shellArguments;
        shellArguments.Push(String(SHELL_PATH));
        shellArguments.Push(String("--login"));

        Vector<String> shellEnvironment;
        shellEnvironment.Push(String("PATH=/usr/bin"));
        shellEnvironment.Push(String("HOME=/root"));
        shellEnvironment.Push(String("TERM=linux"));

        Scheduler::CreateTaskFromELF(String(SHELL_PATH), shellArguments, shellEnvironment);
    }

    Scheduler::StartCores(tss);

    while (true) asm("hlt");
}

extern "C" __attribute__((unused)) void __cxa_pure_virtual()
{
    Serial::Log("__cxa_pure_virtual called");
    Panic();
}