#include "IDT.h"
#include <stdint.h>
#include "APIC.h"
#include "PIC.h"
#include "Serial.h"
#include "Scheduler.h"
#include "VFS.h"
#include "KernelUtilities.h"

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

struct IDT
{
    IDTGateDescriptor entries[256];
    void SetInterruptHandler(int interrupt, uint64_t handler, uint8_t ring = 0, uint8_t ist = 0);
} __attribute__((packed));

struct IDTR
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static IDTR idtr;
static IDT idt;
static int initializedInterruptHandlersCount = 0;

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

// Local APIC IRQs
extern "C" void ISRWrapper48();
extern "C" void ISRWrapper255();

// Miscellaneous
extern "C" void ISRWrapper128();

void ExceptionHandler(InterruptFrame* interruptFrame)
{
    Serial::Printf("Exception %x occurred.", interruptFrame->interruptNumber);
    Serial::Printf("Error code: %x", interruptFrame->errorCode);
    Serial::Print("Hanging...");
    while (true) asm("cli\n hlt");
}

void KeyboardInterruptHandler()
{
    Serial::Printf("Keyboard interrupt: %x", inb(0x60));
    PICSendEIO(1);
}

void LAPICTimerInterrupt(InterruptFrame* interruptFrame)
{
    Task nextTask = GetNextTask(*interruptFrame);

    Serial::Printf("\n------------------------------------------ OLD RIP: %x", interruptFrame->rip);
    Serial::Printf("Count: %d", taskList->GetLength());
    Serial::Printf("------------------------------------------ NEW RIP: %x", nextTask.frame.rip);

    *interruptFrame = nextTask.frame;
    nextTask.pagingManager->SetCR3();

    LAPICSendEOI();
}

void SystemCall(InterruptFrame* interruptFrame)
{
    const char* path = "/subdirectory-bravo/bar.txt";
    VFS::Open(path);
    Serial::Print((char*)interruptFrame->rdi, "");
}

extern "C" void ISRHandler(InterruptFrame* interruptFrame)
{
    switch (interruptFrame->interruptNumber)
    {
        case 48:
            LAPICTimerInterrupt(interruptFrame);
            break;
        case 0 ... 31:
            ExceptionHandler(interruptFrame);
            break;
        case 32 + 1:
            KeyboardInterruptHandler();
            break;
        case 0x80:
            SystemCall(interruptFrame);
            break;
        default:
            Serial::Printf("Could not find ISR for interrupt %x.", interruptFrame->interruptNumber);
            Serial::Print("Hanging...");
            while (true) asm("cli\n hlt");
    }
}

void InitializeInterruptHandlers()
{
    // Exceptions
    idt.SetInterruptHandler(0, reinterpret_cast<uint64_t>(ISRWrapper0));
    idt.SetInterruptHandler(1, reinterpret_cast<uint64_t>(ISRWrapper1), 0, 4); // Debug
    idt.SetInterruptHandler(2, reinterpret_cast<uint64_t>(ISRWrapper2), 0, 2); // Non-maskable Interrupt
    idt.SetInterruptHandler(3, reinterpret_cast<uint64_t>(ISRWrapper3));
    idt.SetInterruptHandler(4, reinterpret_cast<uint64_t>(ISRWrapper4));
    idt.SetInterruptHandler(5, reinterpret_cast<uint64_t>(ISRWrapper5));
    idt.SetInterruptHandler(6, reinterpret_cast<uint64_t>(ISRWrapper6));
    idt.SetInterruptHandler(7, reinterpret_cast<uint64_t>(ISRWrapper7));
    idt.SetInterruptHandler(8, reinterpret_cast<uint64_t>(ISRWrapper8), 0, 1); // Double Fault
    idt.SetInterruptHandler(9, reinterpret_cast<uint64_t>(ISRWrapper9));
    idt.SetInterruptHandler(10, reinterpret_cast<uint64_t>(ISRWrapper10));
    idt.SetInterruptHandler(11, reinterpret_cast<uint64_t>(ISRWrapper11));
    idt.SetInterruptHandler(12, reinterpret_cast<uint64_t>(ISRWrapper12));
    idt.SetInterruptHandler(13, reinterpret_cast<uint64_t>(ISRWrapper13));
    idt.SetInterruptHandler(14, reinterpret_cast<uint64_t>(ISRWrapper14));
    idt.SetInterruptHandler(16, reinterpret_cast<uint64_t>(ISRWrapper16));
    idt.SetInterruptHandler(17, reinterpret_cast<uint64_t>(ISRWrapper17));
    idt.SetInterruptHandler(18, reinterpret_cast<uint64_t>(ISRWrapper18), 0, 3); // Machine Check
    idt.SetInterruptHandler(19, reinterpret_cast<uint64_t>(ISRWrapper19));
    idt.SetInterruptHandler(20, reinterpret_cast<uint64_t>(ISRWrapper20));
    idt.SetInterruptHandler(21, reinterpret_cast<uint64_t>(ISRWrapper21));
    idt.SetInterruptHandler(28, reinterpret_cast<uint64_t>(ISRWrapper28));
    idt.SetInterruptHandler(29, reinterpret_cast<uint64_t>(ISRWrapper29));
    idt.SetInterruptHandler(30, reinterpret_cast<uint64_t>(ISRWrapper30));

    // PIC IRQs
    idt.SetInterruptHandler(32, reinterpret_cast<uint64_t>(ISRWrapper32));
    idt.SetInterruptHandler(33, reinterpret_cast<uint64_t>(ISRWrapper33));
    idt.SetInterruptHandler(34, reinterpret_cast<uint64_t>(ISRWrapper34));
    idt.SetInterruptHandler(35, reinterpret_cast<uint64_t>(ISRWrapper35));
    idt.SetInterruptHandler(36, reinterpret_cast<uint64_t>(ISRWrapper36));
    idt.SetInterruptHandler(37, reinterpret_cast<uint64_t>(ISRWrapper37));
    idt.SetInterruptHandler(38, reinterpret_cast<uint64_t>(ISRWrapper38));
    idt.SetInterruptHandler(39, reinterpret_cast<uint64_t>(ISRWrapper39));
    idt.SetInterruptHandler(40, reinterpret_cast<uint64_t>(ISRWrapper40));
    idt.SetInterruptHandler(41, reinterpret_cast<uint64_t>(ISRWrapper41));
    idt.SetInterruptHandler(42, reinterpret_cast<uint64_t>(ISRWrapper42));
    idt.SetInterruptHandler(43, reinterpret_cast<uint64_t>(ISRWrapper43));
    idt.SetInterruptHandler(44, reinterpret_cast<uint64_t>(ISRWrapper44));
    idt.SetInterruptHandler(45, reinterpret_cast<uint64_t>(ISRWrapper45));
    idt.SetInterruptHandler(46, reinterpret_cast<uint64_t>(ISRWrapper46));
    idt.SetInterruptHandler(47, reinterpret_cast<uint64_t>(ISRWrapper47));

    // Local APIC IRQs
    idt.SetInterruptHandler(48, reinterpret_cast<uint64_t>(ISRWrapper48));
    idt.SetInterruptHandler(255, reinterpret_cast<uint64_t>(ISRWrapper255));

    // Miscellaneous
    idt.SetInterruptHandler(0x80, reinterpret_cast<uint64_t>(ISRWrapper128), 3);
}

void InitializeIDT()
{
    Serial::Print("Initializing IDTR...");

    idtr.base = (uint64_t)&idt;
    idtr.limit = sizeof(idt) - 1;
    InitializeInterruptHandlers();

    Serial::Printf("IDT contains %d initialized Interrupt Handlers (ISRs).", initializedInterruptHandlersCount);
}

void LoadIDT()
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

    initializedInterruptHandlersCount++;
}

