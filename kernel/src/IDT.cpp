#include "IDT.h"

static IDTR idtr;
static IDT idt;

static int initializedInterruptHandlersCount = 0;

void LoadIDT()
{
    Serial::Print("Initializing IDTR...");
    idtr.base = (uint64_t)&idt;
    idtr.limit = sizeof(idt) - 1;

    Serial::Printf("IDT base: %x", idtr.base);
    Serial::Printf("IDT limit: %x", idtr.limit);
    Serial::Printf("IDT contains %d entries ", sizeof(IDT) / sizeof(IDTGateDescriptor), "");
    Serial::Printf("that are each %d bytes in size.", sizeof(IDTGateDescriptor));

    Serial::Print("Initializing Interrupt Handlers (ISRs)...");
    initializedInterruptHandlersCount = 0;
    //idt.SetInterruptHandler(0, reinterpret_cast<uint64_t>(InterruptHandlers::ISR0));
    Serial::Printf("IDT contains %d initialized Interrupt Handlers (ISRs).", initializedInterruptHandlersCount);

    Serial::Print("Loading IDT...");
    asm ( "lidt %0" : : "m"(idtr) );

    /*
    Serial::Print("Doing divide by zero test...");
    asm volatile("div %0" :: "r"(0));

    Serial::Print("Returned from divide by zero test.");*/

    Serial::Print("Completed loading of IDT.\n");
}

void IDT::SetInterruptHandler(int interrupt, uint64_t handler)
{
    IDTGateDescriptor* idtGateDescriptor = &idt.entries[interrupt];

    idtGateDescriptor->offset0 = handler; // 0..15
    idtGateDescriptor->offset1 = handler >> 16; // 16..31
    idtGateDescriptor->offset2 = handler >> 32; // 32..63

    // Bits 0..1: Ring Privilege Level
    // Bit 2: If this is set, this segment is for an LDT. If not, this segment is for a GDT.
    // Bits 3..15: Index of the descriptor in the GDT or LDT.
    // This segment selector describes a segment in Ring 0 that has its descriptor in index 5 of the GDT.
    idtGateDescriptor->segmentSelector = 0b101'0'00;

    // Bits 0..3: Gate Type. 0xe for 64-bit Interrupt Gate and 0xf for 64-bit Trap Gate.
    // Bit 4: Reserved.
    // Bits 5..6: Descriptor Privilege Level
    // Bit 7: Present (P) bit. This must be set for the descriptor to be valid.
    idtGateDescriptor->typeAttributes = 0x8f;

    initializedInterruptHandlersCount++;
}

