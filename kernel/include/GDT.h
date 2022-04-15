#pragma once

#include <stdint.h>

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

class GDT
{
public:
    static void Initialize();
    static TSS* InitializeTSS();
    static void LoadGDTR();
    static void LoadTSS();
private:
    struct SegmentDescriptor
    {
        uint16_t limit0;
        uint16_t base0;
        uint8_t base1;
        uint8_t typeAttributes;
        uint8_t limit1Attributes;
        uint8_t base2;
    } __attribute__((packed));
    SegmentDescriptor entries[7];
} __attribute__((packed));
