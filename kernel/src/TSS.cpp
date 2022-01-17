#include "Memory/Memory.h"
#include "Memory/PageFrameAllocator.h"
#include "TSS.h"
#include "Heap.h"
#include "GDT.h"
#include <stdint.h>
#include "Serial.h"

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

void InitializeTSS()
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

    asm volatile("ltr %0" : : "r"((uint16_t)0b11'000));
    Serial::Print("Initialized TSS.");
}