#include "CPU.h"

uint32_t CPU::GetCoreID()
{
    uint32_t coreId = 0;
    //asm volatile("rdtscp" : "=c"(coreId));

    return coreId;
}