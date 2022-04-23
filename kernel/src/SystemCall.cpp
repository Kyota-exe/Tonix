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

        case SystemCallType::Exit:
            scheduler->ExitCurrentTask((int)arg0, interruptFrame); return 0;

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
            *buffer = scheduler->currentTask.vfs->GetVnodeInfo(fd);
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

        default:
            Panic();
    }
}