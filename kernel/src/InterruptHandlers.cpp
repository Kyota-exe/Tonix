#include "Scheduler.h"
#include "Serial.h"
#include "LAPIC.h"
#include "PIC.h"
#include "VFS.h"
#include "Panic.h"
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
    Process nextTask = GetNextTask(*interruptFrame);

    Serial::Printf("\n------------------------------------------ OLD RIP: %x", interruptFrame->rip);
    Serial::Printf("Count: %d", taskList->GetLength());
    Serial::Printf("------------------------------------------ NEW RIP: %x", nextTask.frame.rip);

    *interruptFrame = nextTask.frame;
    nextTask.pagingManager->SetCR3();

    LAPIC::SendEOI();
}

void SystemCallHandler(InterruptFrame* interruptFrame)
{
    // PLACEHOLDER

    Serial::Print("syscall");

    uint64_t returnValue = 0;
    uint64_t arg0 = interruptFrame->rsi;
    uint64_t arg1 = interruptFrame->rdx;
    uint64_t arg2 = interruptFrame->rcx;
//    uint64_t arg3 = interruptFrame->r8;
//    uint64_t arg4 = interruptFrame->r9;

    auto systemCall = (SystemCall)interruptFrame->rdi;
    switch (systemCall)
    {
        case SystemCall::Open:
            returnValue = Open(String((const char*)arg0), (int)arg1);
            break;
        case SystemCall::Read:
            returnValue = Read((int)arg0, (void*)arg1, arg2);
            break;
        case SystemCall::Write:
            returnValue = Write((int)arg0, (const void*)arg1, arg2);
            break;
        case SystemCall::Special:
        {
            String message = String((const char*)arg0);
            String path = String((const char*)arg1);

            int file = Open(path, 0);
            returnValue = Write(file, message.ToCString(), message.GetLength());

            Close(file);
            break;
        }
        default:
            Panic("Invalid syscall (%d).", systemCall);
    }

    interruptFrame->rax = returnValue;
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