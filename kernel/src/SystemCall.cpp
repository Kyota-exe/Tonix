#include "SystemCall.h"
#include "Serial.h"
#include "FileMap.h"
#include "VFS.h"
#include "Scheduler.h"

uint64_t SystemCall(SystemCallType type, uint64_t arg0, uint64_t arg1, uint64_t arg2,
                    InterruptFrame* interruptFrame, Error& error)
{
    Scheduler* scheduler = Scheduler::GetScheduler();

    switch (type)
    {
        case SystemCallType::Open:
        {
            const char* path = reinterpret_cast<const char*>(arg0);

            if (!scheduler->currentTask.pagingManager->AddressIsAccessible(path))
            {
                Panic();
                error = Error::Fault;
                return -1;
            }

            return scheduler->currentTask.vfs->Open(String(path), (int)arg1, error);
        }

        case SystemCallType::Read:
        {
            void* buffer = reinterpret_cast<void*>(arg1);

            if (!scheduler->currentTask.pagingManager->AddressIsAccessible(buffer))
            {
                Panic();
                error = Error::Fault;
                return 0;
            }

            return scheduler->currentTask.vfs->Read((int)arg0, buffer, arg2, error);
        }

        case SystemCallType::Write:
        {
            const void* buffer = reinterpret_cast<const void*>(arg1);

            if (!scheduler->currentTask.pagingManager->AddressIsAccessible(buffer))
            {
                Panic();
                error = Error::Fault;
                return 0;
            }

            return scheduler->currentTask.vfs->Write((int)arg0, buffer, arg2, error);
        }

        case SystemCallType::Seek:
            return scheduler->currentTask.vfs->RepositionOffset((int)arg0, arg1, (VFS::SeekType)arg2, error);

        case SystemCallType::Close:
            scheduler->currentTask.vfs->Close((int)arg0); return 0;

        case SystemCallType::FileMap:
            return reinterpret_cast<uintptr_t>(FileMap((void*)arg0, arg1));

        case SystemCallType::Log:
            Serial::Log("%s", arg0); return 0;

        case SystemCallType::Panic:
            Serial::Log("%s", arg0);
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

        case SystemCallType::Exit:
            scheduler->ExitCurrentTask((int)arg0, interruptFrame);
            // We need to return rax here because rax gets set to the return value,
            // and we want rax to be set to the rax of the next task.
            return interruptFrame->rax;

        case SystemCallType::Sleep:
        {
            auto nanoseconds = static_cast<int64_t>(arg1);
            Assert(nanoseconds == 0);
            scheduler->SleepCurrentTask(arg0 * 1000 + nanoseconds / 1'000'000);
            return 0;
        }

        case SystemCallType::FStat:
        {
            int fd = (int)arg0;
            auto buffer = reinterpret_cast<VFS::VnodeInfo*>(arg1);

            if (!scheduler->currentTask.pagingManager->AddressIsAccessible(buffer))
            {
                Panic();
                error = Error::Fault;
                return 0;
            }

            *buffer = scheduler->currentTask.vfs->GetVnodeInfo(fd, error);
            return 0;
        }

        case SystemCallType::SetTerminalSettings:
        {
            scheduler->currentTask.vfs->SetTerminalSettings((int)arg0, (bool)arg1, (bool)arg2, error);
            return 0;
        }

        case SystemCallType::Stat:
        {
            const char* path = reinterpret_cast<const char*>(arg0);

            if (!scheduler->currentTask.pagingManager->AddressIsAccessible(path))
            {
                Panic();
                error = Error::Fault;
                return -1;
            }

            int fd = scheduler->currentTask.vfs->Open(String(path), 0, error);
            if (error != Error::None) return -1;

            auto buffer = reinterpret_cast<VFS::VnodeInfo*>(arg1);
            *buffer = scheduler->currentTask.vfs->GetVnodeInfo(fd, error);
            scheduler->currentTask.vfs->Close(fd);

            return 0;
        }

        case SystemCallType::GetPID:
        {
            return scheduler->currentTask.pid;
        }

        case SystemCallType::GetParentPID:
        {
            return scheduler->currentTask.parentPid;
        }

        case SystemCallType::GetWorkingDirectory:
        {
            char* buffer = reinterpret_cast<char*>(arg0);
            uint64_t bufferSize = arg1;

            if (buffer == nullptr || bufferSize == 0)
            {
                error = Error::InvalidArgument;
                return -1;
            }

            if (!scheduler->currentTask.pagingManager->AddressIsAccessible(buffer))
            {
                Panic();
                error = Error::Fault;
                return -1;
            }

            String workingDirectory = scheduler->currentTask.vfs->GetWorkingDirectory();

            if (bufferSize < workingDirectory.GetLength())
            {
                error = Error::BadRange;
                return -1;
            }

            for (uint64_t i = 0; i < bufferSize; ++i)
            {
                buffer[i] = workingDirectory[i];
            }

            return 0;
        }

        case SystemCallType::Fork:
        {
            return scheduler->ForkCurrentTask(interruptFrame);
        }

        case SystemCallType::Wait:
        {
            return scheduler->WaitForChild(error);
        }

        default:
            Panic();
    }
}