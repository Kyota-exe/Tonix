#include "Scheduler.h"
#include "Serial.h"
#include "LAPIC.h"
#include "PIC.h"
#include "SystemCall.h"

void ExceptionHandler(InterruptFrame* interruptFrame)
{
    Serial::Printf("Error code: %x", interruptFrame->errorCode);
    Panic("Exception %x occurred.", interruptFrame->interruptNumber);
}

void KeyboardInterruptHandler()
{
    Serial::Printf("Keyboard interrupt: %x", inb(0x60));
    PICSendEIO(1);
}

void LAPICTimerInterrupt(InterruptFrame* interruptFrame)
{
	Serial::Print("Timer interrupt");
    LAPIC::SendEOI();

    Process nextTask = GetNextTask(interruptFrame);
    *interruptFrame = nextTask.frame;
    nextTask.pagingManager->SetCR3();
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
            Panic("Could not find ISR for interrupt %x.", interruptFrame->interruptNumber);
    }
}