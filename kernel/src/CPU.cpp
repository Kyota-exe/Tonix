#include "CPU.h"

Vector<CPU>* cpuList;

uint32_t CPU::GetCoreID()
{
    uint32_t coreId;
    asm volatile("rdtscp" : "=c"(coreId) : : "rax", "rdx");

    return coreId;
}

CPU CPU::GetStruct()
{
    return cpuList->Get(GetCoreID());
}

void CPU::EnableSSE()
{
    uint64_t cr4;
    asm volatile("mov %%cr4, %0" : "=r"(cr4));

    uint64_t mask = 1 << 9;
    Assert((cr4 & mask) == 0);

    cr4 |= mask;

    asm volatile("mov %0, %%cr4" : : "r" (cr4));
}

void CPU::InitializeCPUList(unsigned long cpuCount)
{
    Assert(cpuList == nullptr);

    cpuList = new Vector<CPU>();
    for (uint64_t i = 0; i < cpuCount; ++i)
    {
        cpuList->Push({nullptr});
    }
}

void CPU::InitializeCPUStruct(Scheduler* scheduler)
{
    uint32_t coreId = GetCoreID();
    Assert(cpuList->Get(coreId).scheduler == nullptr);
    cpuList->Get(coreId).scheduler = scheduler;
}

void CPU::SetTCB(const void* tcbAddr)
{
    asm volatile("mov %0, %%fs" : : "r"(0b10011));
    uintptr_t value = reinterpret_cast<uintptr_t>(tcbAddr);
    uint32_t low = value & 0xffffffff;
    uint32_t high = value >> 32;
    asm volatile("wrmsr" : : "c"(0xC0000100), "a"(low), "d"(high));
}
