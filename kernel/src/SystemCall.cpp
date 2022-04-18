#include "SystemCall.h"
#include "Serial.h"
#include "FileMap.h"
#include "VFS.h"
#include "Scheduler.h"

uint64_t SystemCall(SystemCallType type, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                    uint64_t arg4, uint64_t arg5, InterruptFrame* interruptFrame, Error& error)
{
    // PLACEHOLDER

    if (type != SystemCallType::Log)
    {
        Serial::Printf("syscall %d", type);
    }

    Scheduler* scheduler = Scheduler::GetScheduler();

    switch (type)
    {
        case SystemCallType::Open:
            return scheduler->currentTask.vfs->Open(String((const char*)arg0), (int)arg1, error);

        case SystemCallType::Read:
            return scheduler->currentTask.vfs->Read((int)arg0, (void*)arg1, arg2);

        case SystemCallType::Write:
            return scheduler->currentTask.vfs->Write((int)arg0, (const void*)arg1, arg2);

        case SystemCallType::Seek:
            return scheduler->currentTask.vfs->RepositionOffset((int)arg0, arg1, (VFS::SeekType)arg2, error);

        case SystemCallType::Close:
            scheduler->currentTask.vfs->Close((int)arg0); return 0;

        case SystemCallType::FileMap:
            return (uint64_t)FileMap(&scheduler->currentTask, (void*)arg0, arg1,
                                     (int)arg2,(int)arg3, (int)arg4, (int)arg5);

        case SystemCallType::Log:
            Serial::Print((const char*)arg0); return 0;

        case SystemCallType::Panic:
            Serial::Print((const char*)arg0);
            Panic();

        case SystemCallType::TCBSet:
        {
            // TODO: Make this better
            asm volatile("mov %0, %%fs" : : "r"(0b10011));
            uint32_t low = arg0 & 0xFFFFFFFF;
            uint32_t high = arg0 >> 32;
            asm volatile("wrmsr" : : "c"(0xC0000100), "a"(low), "d"(high));
            return 0;
        }

        case SystemCallType::IsTerminal:
        {
            VFS::VnodeType vnodeType = scheduler->currentTask.vfs->GetVnodeType((int)arg0, error);

            // Assuming all VFS character devices are terminals
            if (error == Error::None && vnodeType != VFS::VnodeType::CharacterDevice)
            {
                error = Error::NotTerminal;
            }

            return error == Error::None ? 0 : -1;
        }

        case SystemCallType::Exit:
            scheduler->ExitCurrentTask((int)arg0, interruptFrame); return 0;

        case SystemCallType::Sleep:
        {
            auto nanoseconds = static_cast<int64_t>(arg1);
            Assert(nanoseconds == 0);
            scheduler->SleepCurrentTask(arg0 * 1000 + nanoseconds / 1'000'000);
            return 0;
        }

        default:
            Panic();
    }
}