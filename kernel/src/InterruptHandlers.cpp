#include "Scheduler.h"
#include "Serial.h"
#include "LAPIC.h"
#include "PIC.h"
#include "SystemCall.h"

void PageFaultHandler()
{
    Serial::Print("Page fault occurred.");

    uint64_t cr2;
    asm volatile("mov %%cr2, %0" : "=r"(cr2));
    Serial::Printf("CR2: %x", cr2);
}

void ExceptionHandler(InterruptFrame* interruptFrame)
{
    Serial::Printf("Error code: %x", interruptFrame->errorCode);
    Serial::Printf("RIP: %x", interruptFrame->rip);

    if (interruptFrame->interruptNumber == 0xe) PageFaultHandler();
    Panic("Exception %x occurred.", interruptFrame->interruptNumber);
}

void KeyboardInterruptHandler()
{
    Serial::Printf("Keyboard interrupt: %x", inb(0x60));
    PICSendEIO(1);
}

void LAPICTimerInterrupt(InterruptFrame* interruptFrame)
{
	Serial::Print("Timer interrupt---------------------------");
    SwitchToNextTask(interruptFrame);
    LAPIC::SendEOI();
}

void SystemCallHandler(InterruptFrame* interruptFrame)
{
    Error error = Error::None;

    interruptFrame->rax = SystemCall((SystemCallType)interruptFrame->rdi,
                                     interruptFrame->rsi,interruptFrame->rdx,
                                     interruptFrame->rcx,interruptFrame->r8,
                                     interruptFrame->r9,interruptFrame->r10, interruptFrame, error);

    interruptFrame->rbx = (uint64_t)error;
}

extern "C" void ISRHandler(InterruptFrame* interruptFrame)
{
    switch (interruptFrame->interruptNumber)
    {
        case 48:
            LAPICTimerInterrupt(interruptFrame);
            break;
        case 32 + 1:
            KeyboardInterruptHandler();
            break;
        case 0x80:
            SystemCallHandler(interruptFrame);
            break;
        case 0 ... 31:
            ExceptionHandler(interruptFrame);
            break;
        default:
            Panic("Could not find ISR for interrupt %x.", interruptFrame->interruptNumber);
    }
}