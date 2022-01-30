#pragma once

#include <stdint.h>

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
    SegmentDescriptor entries[7];
} __attribute__((packed));

void LoadGDT();
void InitializeGDT();

extern GDT gdt;
