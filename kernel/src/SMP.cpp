#include "SMP.h"
#include "Serial.h"
#include "Stivale2Interface.h"
#include "PageFrameAllocator.h"
#include "APIC.h"
#include "KernelUtilities.h"

static void InitializeCore(stivale2_smp_info* smpInfoPtr)
{
    asm volatile("mov %0, %%cr3" : : "r" (kernelPagingManager.pml4PhysAddr));
    ActivateLAPIC();
    uint64_t lapicTimerFreq = CalibrateLAPICTimer();
    Serial::Printf("%d Hz", lapicTimerFreq);
    while (true) asm("hlt");
}

void StartNonBSPCores()
{
    Serial::Print("Starting non-BSP cores...");
    stivale2_struct_tag_smp* smpStruct = (stivale2_struct_tag_smp*)GetStivale2Tag(STIVALE2_STRUCT_TAG_SMP_ID);
    if (smpStruct->cpu_count > 1)
    {
        Serial::Printf("CPU cores: %d", smpStruct->cpu_count);
        for (uint64_t coreIndex = 0; coreIndex < smpStruct->cpu_count; ++coreIndex)
        {
            stivale2_smp_info* smpInfo = &smpStruct->smp_info[coreIndex];
            if (smpInfo->lapic_id == smpStruct->bsp_lapic_id) continue;
            smpInfo->target_stack = (uint64_t)RequestPageFrame() + 0xffff'8000'0000'0000;
            smpInfo->goto_address = reinterpret_cast<uint64_t>(InitializeCore);
        }
    }
    else
    {
        Serial::Print("Machine has 1 CPU core. SMP is not supported.", "\n\n");
    }
}