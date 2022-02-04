#include "GDT.h"
#include "Memory/Memory.h"
#include "Memory/PageFrameAllocator.h"

struct GDTR
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

struct TSS
{
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t ioMapOffset;
} __attribute__((packed));

constexpr uint8_t KERNEL_CODE_TYPE_ATTRIBUTES = 0b1'00'1'1010;
constexpr uint8_t KERNEL_DATA_TYPE_ATTRIBUTES = 0b1'00'1'0010;
constexpr uint8_t USER_CODE_TYPE_ATTRIBUTES = 0b1'11'1'1010;
constexpr uint8_t USER_DATA_TYPE_ATTRIBUTES = 0b1'11'1'0010;
constexpr uint8_t LIMIT1_ATTRIBUTES = 0b0'01'00000;

GDTR gdtr;
GDT gdt;

void GDT::LoadGDTR()
{
    asm volatile("lgdt %0" : : "m"(gdtr));
}

void GDT::LoadTSS()
{
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

    gdtr.base = (uint64_t)&gdt;
    gdtr.limit = sizeof(gdt) - 1;

    // No need to reload segment registers, since we use the same GDT entry indexes as limine (stivale2)
}

void GDT::InitializeTSS()
{
    auto tss = new TSS();
    Memset(tss, 0, sizeof(TSS));
    tss->rsp0 = (uint64_t)RequestPageFrame() + 0xffff'8000'0000'0000;
    tss->ist1 = (uint64_t)RequestPageFrame() + 0xffff'8000'0000'0000; // Double Fault
    tss->ist2 = (uint64_t)RequestPageFrame() + 0xffff'8000'0000'0000; // Non-Maskable Interrupt
    tss->ist3 = (uint64_t)RequestPageFrame() + 0xffff'8000'0000'0000; // Machine Check
    tss->ist4 = (uint64_t)RequestPageFrame() + 0xffff'8000'0000'0000; // Debug
    tss->ioMapOffset = sizeof(TSS);

    gdt.entries[3].base0 = (uint64_t)tss;
    gdt.entries[3].base1 = (uint64_t)tss >> 16;
    gdt.entries[3].base2 = (uint64_t)tss >> 24;
    gdt.entries[3].limit0 = sizeof(TSS) - 1;
    gdt.entries[3].limit1Attributes = 0b0'0'0'0'0000 | ((sizeof(TSS) - 1) >> 16);
    gdt.entries[3].typeAttributes = 0b1'00'0'1001;
    *(uint64_t*)&gdt.entries[4] = (uint64_t)tss >> 32;
}