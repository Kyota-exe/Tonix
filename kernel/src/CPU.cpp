#include "CPU.h"

uint32_t CPU::GetCoreID()
{
    uint32_t coreId;
    asm volatile("rdtscp" : "=c"(coreId) : : "rax", "rdx");

    return coreId;
}