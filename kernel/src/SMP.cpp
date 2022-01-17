#include "Memory/PageFrameAllocator.h"
#include "SMP.h"
#include "Serial.h"
#include "Stivale2Interface.h"
#include "APIC.h"
#include "IDT.h"
#include "GDT.h"
#include "TSS.h"
#include "KernelUtilities.h"

static void InitializeCore(stivale2_smp_info* smpInfoPtr)
{
    LoadGDT();
    LoadIDT();
    kernelPagingManager.SetCR3();
    InitializeTSS();

    // The address of the smpInfoPtr stivale2 passes us is physical, so we convert it to virtual here so that we can access it.
    smpInfoPtr = (stivale2_smp_info*)((uint64_t)smpInfoPtr + 0xffff'8000'0000'0000);

    StartScheduler();

    while (true) asm("hlt");
}

void StartSchedulerOnNonBSPCores()
{
    auto smpStruct = (stivale2_struct_tag_smp*)GetStivale2Tag(STIVALE2_STRUCT_TAG_SMP_ID);

    Serial::Printf("SMP STRUCT: %x", (uint64_t)smpStruct);
    Serial::Printf("CPU cores: %d", smpStruct->cpu_count);
    if (smpStruct->cpu_count > 1)
    {
        for (uint64_t coreIndex = 0; coreIndex < smpStruct->cpu_count; ++coreIndex)
        {
            stivale2_smp_info* smpInfo = &smpStruct->smp_info[coreIndex];
            if (coreIndex == 2) Serial::Printf("INFO addr: %x", (uint64_t)smpInfo);
            if (smpInfo->lapic_id == smpStruct->bsp_lapic_id) continue;
            smpInfo->target_stack = (uint64_t)RequestPageFrame() + 0xffff'8000'0000'0000;
            smpInfo->goto_address = reinterpret_cast<uint64_t>(InitializeCore);
        }
    }
}