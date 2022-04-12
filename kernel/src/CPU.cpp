#include "CPU.h"

uint32_t CPU::GetCoreID()
{
    uint32_t coreId;
    asm volatile("rdtscp" : "=c"(coreId) : : "rax", "rdx");

    return coreId;
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