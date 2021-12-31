#ifndef MISKOS_IDT_H
#define MISKOS_IDT_H

#include <stdint.h>
#include "Serial.h"

void LoadIDT();

struct IDTGateDescriptor
{
    uint16_t offset0;
    uint16_t segmentSelector;
    uint8_t reserved0;
    uint8_t typeAttributes;
    uint16_t offset1;
    uint32_t offset2;
    uint32_t reserved1;
} __attribute__((packed));

struct IDT
{
    IDTGateDescriptor entries[256];
    void SetInterruptHandler(int interrupt, uint64_t handler, int &initializedInterruptHandlersCount);
} __attribute__((packed));

struct IDTR
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

#endif
