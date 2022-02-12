#include "Scheduler.h"
#include "Serial.h"
#include "LAPIC.h"
#include "PIC.h"
#include "SystemCall.h"

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
	Serial::Print("Timer");
    Process nextTask = GetNextTask(*interruptFrame);

    *interruptFrame = nextTask.frame;
	Serial::Print("\n\nReturned");
	Serial::Printf("RIP: %x", interruptFrame->rip);
	Serial::Printf("Yes? %d", nextTask.pagingManager->FlagMismatchLevel((void*)interruptFrame->rip, PagingFlag::Present, true));
    nextTask.pagingManager->SetCR3();

    LAPIC::SendEOI();
}

void SystemCallHandler(InterruptFrame* interruptFrame)
{
    Error error = Error::None;

    interruptFrame->rax = SystemCall((SystemCallType)interruptFrame->rdi,
                                     interruptFrame->rsi,interruptFrame->rdx,
                                     interruptFrame->rcx,interruptFrame->r8,
                                     interruptFrame->r9,interruptFrame->r10, error);

    interruptFrame->rbx = (uint64_t)error;
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
            SystemCallHandler(interruptFrame);
            break;
        default:
            Serial::Printf("Could not find ISR for interrupt %x.", interruptFrame->interruptNumber);
            Serial::Print("Hanging...");
            while (true) asm("cli\n hlt");
    }
}