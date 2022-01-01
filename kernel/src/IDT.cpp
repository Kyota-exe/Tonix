#include "IDT.h"

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

struct InterruptFrame
{
    uint64_t es;
    uint64_t ds;
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
    uint64_t errorCode;
} __attribute__((packed));

static IDTR idtr;
static IDT idt;
static uint8_t errorCodeInterruptNumbers[] { 0x08, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x11, 0x15, 0x1d, 0x1e };

extern "C" void ISRWrapperNoErrorCode();
extern "C" void ISRWrapperErrorCode();

extern "C" void ISRHandler(InterruptFrame* interruptFrame)
{
    Serial::Printf("Exception occurred. Error Code: %x", interruptFrame->errorCode);
    while (true) asm volatile("hlt");
}

void LoadIDT()
{
    Serial::Print("Initializing IDTR...");
    idtr.base = (uint64_t)&idt;
    idtr.limit = sizeof(idt) - 1;

    Serial::Printf("IDT base: %x", idtr.base);
    Serial::Printf("IDT limit: %x", idtr.limit);
    Serial::Printf("IDT size: %x", idtr.limit + 1);
    Serial::Printf("IDT contains %d entries ", sizeof(IDT) / sizeof(IDTGateDescriptor), "");
    Serial::Printf("that are each %d bytes in size.", sizeof(IDTGateDescriptor));

    Serial::Print("Initializing Interrupt Handlers (ISRs)...");
    int initializedInterruptHandlersCount = 0;
    for (uint8_t interruptNum = 0; interruptNum <= 31; ++interruptNum)
    {
        bool hasErrorCode = false;
        for (uint8_t n : errorCodeInterruptNumbers) if (interruptNum == n) hasErrorCode = true;
        uint64_t isrWrapperAddr = reinterpret_cast<uint64_t>(hasErrorCode ? ISRWrapperErrorCode : ISRWrapperNoErrorCode);
        idt.SetInterruptHandler(interruptNum, isrWrapperAddr, initializedInterruptHandlersCount);
    }
    Serial::Printf("IDT contains %d initialized Interrupt Handlers (ISRs).", initializedInterruptHandlersCount);

    Serial::Print("Loading IDT...");
    asm("lidt %0" : : "m"(idtr));

    Serial::Print("Completed loading of IDT.", "\n\n");
}

void IDT::SetInterruptHandler(int interrupt, uint64_t handler, int &initializedInterruptHandlersCount)
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
    idtGateDescriptor->typeAttributes = 0x8e;

    initializedInterruptHandlersCount++;
}

