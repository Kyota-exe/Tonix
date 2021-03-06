#include "GDT.h"
#include "Memory/Memory.h"

constexpr uint8_t KERNEL_CODE_TYPE_ATTRIBUTES = 0b1'00'1'1010;
constexpr uint8_t KERNEL_DATA_TYPE_ATTRIBUTES = 0b1'00'1'0010;
constexpr uint8_t USER_CODE_TYPE_ATTRIBUTES = 0b1'11'1'1010;
constexpr uint8_t USER_DATA_TYPE_ATTRIBUTES = 0b1'11'1'0010;
constexpr uint8_t LIMIT1_ATTRIBUTES = 0b0'01'00000;
constexpr uint8_t TSS_TYPE_ATTRIBUTES = 0b1'00'0'1001;

struct GDTR
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

GDTR gdtr;
GDT gdt;

void GDT::LoadGDTR()
{
    asm volatile("lgdt %0" : : "m"(gdtr));
}

void GDT::LoadTSS(TSS* tss)
{
    auto tssAddr = reinterpret_cast<uintptr_t>(tss);

    gdt.entries[3].base0 = tssAddr;
    gdt.entries[3].base1 = tssAddr >> 16;
    gdt.entries[3].base2 = tssAddr >> 24;
    gdt.entries[3].limit0 = sizeof(TSS) - 1;
    gdt.entries[3].limit1Attributes = (sizeof(TSS) - 1) >> 16;
    gdt.entries[3].typeAttributes = TSS_TYPE_ATTRIBUTES;
    *(uint64_t*)&gdt.entries[4] = tssAddr >> 32;

    asm volatile("ltr %0" : : "r"((uint16_t)0b11'000));
}

void GDT::Initialize()
{
    // User Code Segment
    gdt.entries[1].typeAttributes = USER_CODE_TYPE_ATTRIBUTES;
    gdt.entries[1].limit1Attributes = LIMIT1_ATTRIBUTES;

    // User Data Segment
    gdt.entries[2].typeAttributes = USER_DATA_TYPE_ATTRIBUTES;
    gdt.entries[2].limit1Attributes = LIMIT1_ATTRIBUTES;

    // Kernel Code Segment
    gdt.entries[5].typeAttributes = KERNEL_CODE_TYPE_ATTRIBUTES;
    gdt.entries[5].limit1Attributes = LIMIT1_ATTRIBUTES;

    // Kernel Data Segment
    gdt.entries[6].typeAttributes = KERNEL_DATA_TYPE_ATTRIBUTES;
    gdt.entries[6].limit1Attributes = LIMIT1_ATTRIBUTES;

    gdtr.base = (uintptr_t)&gdt;
    gdtr.limit = sizeof(gdt) - 1;

    // No need to reload segment registers, since we use the same GDT entry indexes as limine (stivale2)
}