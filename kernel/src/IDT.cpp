#include "IDT.h"

// Exceptions
extern "C" void ISRWrapper0();
extern "C" void ISRWrapper1();
extern "C" void ISRWrapper2();
extern "C" void ISRWrapper3();
extern "C" void ISRWrapper4();
extern "C" void ISRWrapper5();
extern "C" void ISRWrapper6();
extern "C" void ISRWrapper7();
extern "C" void ISRWrapper8();
extern "C" void ISRWrapper9();
extern "C" void ISRWrapper10();
extern "C" void ISRWrapper11();
extern "C" void ISRWrapper12();
extern "C" void ISRWrapper13();
extern "C" void ISRWrapper14();
extern "C" void ISRWrapper16();
extern "C" void ISRWrapper17();
extern "C" void ISRWrapper18();
extern "C" void ISRWrapper19();
extern "C" void ISRWrapper20();
extern "C" void ISRWrapper21();
extern "C" void ISRWrapper28();
extern "C" void ISRWrapper29();
extern "C" void ISRWrapper30();

// PIC IRQs
extern "C" void ISRWrapper32();
extern "C" void ISRWrapper33();
extern "C" void ISRWrapper34();
extern "C" void ISRWrapper35();
extern "C" void ISRWrapper36();
extern "C" void ISRWrapper37();
extern "C" void ISRWrapper38();
extern "C" void ISRWrapper39();
extern "C" void ISRWrapper40();
extern "C" void ISRWrapper41();
extern "C" void ISRWrapper42();
extern "C" void ISRWrapper43();
extern "C" void ISRWrapper44();
extern "C" void ISRWrapper45();
extern "C" void ISRWrapper46();
extern "C" void ISRWrapper47();

// Local LAPIC IRQs
extern "C" void ISRWrapper48();
extern "C" void ISRWrapper255();

// Miscellaneous
extern "C" void ISRWrapper128();
extern "C" void ISRWrapper129();

struct IDTGateDescriptor
{
    uint16_t offset0;
    uint16_t segmentSelector;
    uint8_t ist;
    uint8_t typeAttributes;
    uint16_t offset1;
    uint32_t offset2;
    uint32_t reserved1;
} __attribute__((packed));

struct IDTR
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

IDTR idtr;
IDTGateDescriptor entries[256];

void IDT::InitializeInterruptHandlers()
{
    // Exceptions
    SetInterruptHandler(0, reinterpret_cast<uint64_t>(ISRWrapper0));
    SetInterruptHandler(1, reinterpret_cast<uint64_t>(ISRWrapper1), 0, 4); // Debug
    SetInterruptHandler(2, reinterpret_cast<uint64_t>(ISRWrapper2), 0, 2); // Non-maskable Interrupt
    SetInterruptHandler(3, reinterpret_cast<uint64_t>(ISRWrapper3));
    SetInterruptHandler(4, reinterpret_cast<uint64_t>(ISRWrapper4));
    SetInterruptHandler(5, reinterpret_cast<uint64_t>(ISRWrapper5));
    SetInterruptHandler(6, reinterpret_cast<uint64_t>(ISRWrapper6));
    SetInterruptHandler(7, reinterpret_cast<uint64_t>(ISRWrapper7));
    SetInterruptHandler(8, reinterpret_cast<uint64_t>(ISRWrapper8), 0, 1); // Double Fault
    SetInterruptHandler(9, reinterpret_cast<uint64_t>(ISRWrapper9));
    SetInterruptHandler(10, reinterpret_cast<uint64_t>(ISRWrapper10));
    SetInterruptHandler(11, reinterpret_cast<uint64_t>(ISRWrapper11));
    SetInterruptHandler(12, reinterpret_cast<uint64_t>(ISRWrapper12));
    SetInterruptHandler(13, reinterpret_cast<uint64_t>(ISRWrapper13));
    SetInterruptHandler(14, reinterpret_cast<uint64_t>(ISRWrapper14));
    SetInterruptHandler(16, reinterpret_cast<uint64_t>(ISRWrapper16));
    SetInterruptHandler(17, reinterpret_cast<uint64_t>(ISRWrapper17));
    SetInterruptHandler(18, reinterpret_cast<uint64_t>(ISRWrapper18), 0, 3); // Machine Check
    SetInterruptHandler(19, reinterpret_cast<uint64_t>(ISRWrapper19));
    SetInterruptHandler(20, reinterpret_cast<uint64_t>(ISRWrapper20));
    SetInterruptHandler(21, reinterpret_cast<uint64_t>(ISRWrapper21));
    SetInterruptHandler(28, reinterpret_cast<uint64_t>(ISRWrapper28));
    SetInterruptHandler(29, reinterpret_cast<uint64_t>(ISRWrapper29));
    SetInterruptHandler(30, reinterpret_cast<uint64_t>(ISRWrapper30));

    // PIC IRQs
    SetInterruptHandler(32, reinterpret_cast<uint64_t>(ISRWrapper32));
    SetInterruptHandler(33, reinterpret_cast<uint64_t>(ISRWrapper33));
    SetInterruptHandler(34, reinterpret_cast<uint64_t>(ISRWrapper34));
    SetInterruptHandler(35, reinterpret_cast<uint64_t>(ISRWrapper35));
    SetInterruptHandler(36, reinterpret_cast<uint64_t>(ISRWrapper36));
    SetInterruptHandler(37, reinterpret_cast<uint64_t>(ISRWrapper37));
    SetInterruptHandler(38, reinterpret_cast<uint64_t>(ISRWrapper38));
    SetInterruptHandler(39, reinterpret_cast<uint64_t>(ISRWrapper39));
    SetInterruptHandler(40, reinterpret_cast<uint64_t>(ISRWrapper40));
    SetInterruptHandler(41, reinterpret_cast<uint64_t>(ISRWrapper41));
    SetInterruptHandler(42, reinterpret_cast<uint64_t>(ISRWrapper42));
    SetInterruptHandler(43, reinterpret_cast<uint64_t>(ISRWrapper43));
    SetInterruptHandler(44, reinterpret_cast<uint64_t>(ISRWrapper44));
    SetInterruptHandler(45, reinterpret_cast<uint64_t>(ISRWrapper45));
    SetInterruptHandler(46, reinterpret_cast<uint64_t>(ISRWrapper46));
    SetInterruptHandler(47, reinterpret_cast<uint64_t>(ISRWrapper47));

    // Local LAPIC IRQs
    SetInterruptHandler(48, reinterpret_cast<uint64_t>(ISRWrapper48), 3, 5); // Timer Interrupt
    SetInterruptHandler(255, reinterpret_cast<uint64_t>(ISRWrapper255));

    // Miscellaneous
    SetInterruptHandler(0x80, reinterpret_cast<uint64_t>(ISRWrapper128), 3, 7); // System Call
    SetInterruptHandler(0x81, reinterpret_cast<uint64_t>(ISRWrapper129));
}

void IDT::Initialize()
{
    idtr.base = (uint64_t)entries;
    idtr.limit = sizeof(entries) - 1;
    InitializeInterruptHandlers();
}

void IDT::Load()
{
    asm volatile("lidt %0" : : "m"(idtr));
}

void IDT::SetInterruptHandler(int interrupt, uint64_t handler, uint8_t ring, uint8_t ist)
{
    IDTGateDescriptor* idtGateDescriptor = &entries[interrupt];

    idtGateDescriptor->offset0 = handler; // 0..15
    idtGateDescriptor->offset1 = handler >> 16; // 16..31
    idtGateDescriptor->offset2 = handler >> 32; // 32..63

    idtGateDescriptor->ist = ist;

    // Bits 0..1: Ring Privilege Level
    // Bit 2: If this is set, this segment is for an LDT. If not, this segment is for a GDT.
    // Bits 3..15: Index of the descriptor in the GDT or LDT.
    // This segment selector represents the kernel code segment.
    idtGateDescriptor->segmentSelector = 0b101'0'00;

    // Bits 0..3: Gate Type. 0b1110 for 64-bit Interrupt Gate and 0b1111 for 64-bit Trap Gate.
    // Bit 4: Reserved.
    // Bits 5..6: Descriptor Privilege Level
    // Bit 7: Present (P) bit. This must be set for the descriptor to be valid.
    idtGateDescriptor->typeAttributes = 0b1'00'0'1110 | ring << 5;
}
