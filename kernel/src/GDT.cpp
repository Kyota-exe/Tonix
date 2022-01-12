#include "GDT.h"
#include <stdint.h>
#include "Serial.h"

struct SegmentDescriptor
{
    uint16_t limit0;
    uint16_t base0;
    uint8_t base1;
    uint8_t typeAttributes;
    uint8_t limit1Attributes;
    uint8_t base2;
} __attribute__((packed));

struct GDT
{
    SegmentDescriptor entries[5];
} __attribute__((packed));

struct GDTR
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static GDTR gdtr;
static GDT gdt;

const uint8_t KERNEL_CODE_TYPE_ATTRIBUTES = 0b1'00'1'1010;
const uint8_t KERNEL_DATA_TYPE_ATTRIBUTES = 0b1'00'1'0010;
const uint8_t USER_CODE_TYPE_ATTRIBUTES = 0b1'11'1'1010;
const uint8_t USER_DATA_TYPE_ATTRIBUTES = 0b1'11'1'0010;
const uint8_t LIMIT1_ATTRIBUTES = 0b0'01'00000;
const uint64_t STIVALE2_GDT_SIZE = 8 * 7;

void InitializeGDT()
{
    GDTR oldGdtr;
    asm volatile("sgdt %0" : "=m"(oldGdtr));
    Serial::Printf("Old GDT base: %x", oldGdtr.base);

    // Kernel Code Segment
    gdt.entries[1].typeAttributes = KERNEL_CODE_TYPE_ATTRIBUTES;
    gdt.entries[1].limit1Attributes = LIMIT1_ATTRIBUTES;

    // Kernel Data Segment
    gdt.entries[2].typeAttributes = KERNEL_DATA_TYPE_ATTRIBUTES;
    gdt.entries[2].limit1Attributes = LIMIT1_ATTRIBUTES;

    // User Code Segment
    gdt.entries[3].typeAttributes = USER_CODE_TYPE_ATTRIBUTES;
    gdt.entries[3].limit1Attributes = LIMIT1_ATTRIBUTES;

    // User Data Segment
    gdt.entries[4].typeAttributes = USER_DATA_TYPE_ATTRIBUTES;
    gdt.entries[4].limit1Attributes = LIMIT1_ATTRIBUTES;

    gdtr.base = (uint64_t)&gdt;
    gdtr.limit = sizeof(gdt) - 1;

    Serial::Printf("GDT %x", gdtr.base);
    asm volatile("lgdt %0" : : "m"(gdtr));
}