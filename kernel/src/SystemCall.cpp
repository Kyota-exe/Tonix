#include "SystemCall.h"
#include "Serial.h"
#include "FileMap.h"
#include "VFS.h"
#include "Scheduler.h"
#include "CPU.h"

uint64_t SystemCall(SystemCallType type, uint64_t arg0, uint64_t arg1, uint64_t arg2,
                    InterruptFrame* interruptFrame, Error& error)
{
    Scheduler* scheduler = Scheduler::GetScheduler();

    switch (type)
    {
        case SystemCallType::Open:
        {
            const char* path = reinterpret_cast<const char*>(arg0);
            return scheduler->currentTask.vfs->Open(String(path), (int)arg1, error);
        }

        case SystemCallType::Read:
        {
            void* buffer = reinterpret_cast<void*>(arg1);
            return scheduler->currentTask.vfs->Read((int)arg0, buffer, arg2, error);
        }

        case SystemCallType::Write:
        {
            const void* buffer = reinterpret_cast<const void*>(arg1);
            return scheduler->currentTask.vfs->Write((int)arg0, buffer, arg2, error);
        }

        case SystemCallType::Seek:
            return scheduler->currentTask.vfs->RepositionOffset(static_cast<int>(arg0), static_cast<int64_t>(arg1),
                                                                static_cast<VFS::SeekType>(arg2), error);

        case SystemCallType::Close:
            scheduler->currentTask.vfs->Close(static_cast<int>(arg0), error);
            return 0;

        case SystemCallType::FileMap:
            return reinterpret_cast<uintptr_t>(FileMap((void*)arg0, arg1));

        case SystemCallType::Log:
            Serial::Log("%s", arg0); return 0;

        case SystemCallType::Panic:
            Serial::Log("%s", arg0);
            Panic();

        case SystemCallType::TCBSet:
        {
            void* taskControlBlock = reinterpret_cast<void*>(arg0);
            scheduler->currentTask.taskControlBlock = taskControlBlock;
            CPU::SetTCB(taskControlBlock);
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

            *buffer = scheduler->currentTask.vfs->GetVnodeInfo(fd, error);
            return 0;
        }

        case SystemCallType::SetTerminalSettings:
        {
            scheduler->currentTask.vfs->SetTerminalSettings((int)arg0, (bool)arg1, (bool)arg2, error);
            return 0;
        }

        case SystemCallType::GetTerminalWindowSize:
        {
            auto windowSize = reinterpret_cast<WindowSize*>(arg1);
            *windowSize = scheduler->currentTask.vfs->GetTerminalWindowSize((int)arg0, error);
            return 0;
        }

        case SystemCallType::Stat:
        {
            const char* path = reinterpret_cast<const char*>(arg0);

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

            String workingDirectory = scheduler->currentTask.vfs->GetWorkingDirectory();

            // Chop off the last character '/'
            Assert(workingDirectory[workingDirectory.GetLength() - 1] == '/');
            workingDirectory = workingDirectory.Substring(0, workingDirectory.GetLength() - 1);

            if (bufferSize < workingDirectory.GetLength())
            {
                error = Error::BadRange;
                return -1;
            }

            for (uint64_t i = 0; i < workingDirectory.GetLength(); ++i)
            {
                buffer[i] = workingDirectory[i];
            }

            return 0;
        }

        case SystemCallType::SetWorkingDirectory:
        {
            const char* newPath = reinterpret_cast<const char*>(arg0);
            scheduler->currentTask.vfs->SetWorkingDirectory(String(newPath), error);
            return 0;
        }

        case SystemCallType::Fork:
        {
            return scheduler->ForkCurrentTask(interruptFrame);
        }

        case SystemCallType::Execute:
        {
            String path = String(reinterpret_cast<const char*>(arg0));

            Vector<String> arguments;
            auto argumentsPtr = reinterpret_cast<char* const*>(arg1);
            while (*argumentsPtr != nullptr)
            {
                arguments.Push(String(*argumentsPtr++));
            }

            Vector<String> environment;
            auto environmentPtr = reinterpret_cast<char* const*>(arg2);
            while (*environmentPtr != nullptr)
            {
                environment.Push(String(*environmentPtr++));
            }

            scheduler->Execute(path, interruptFrame, arguments, environment, error);
            return interruptFrame->rax;
        }

        case SystemCallType::Wait:
        {
            int* status = reinterpret_cast<int*>(arg1);
            return scheduler->WaitForChild(arg0, *status, error);
        }

        case SystemCallType::ReadDirectory:
        {
            VFS::DirectoryEntry directoryEntry {};
            scheduler->currentTask.vfs->Read((int)arg0, &directoryEntry, sizeof(directoryEntry), error);

            if (error != Error::None) return -1;

            struct DirectoryEntryInfo
            {
                uint32_t inodeNum;
                VFS::VnodeType type;
                char name[1024];
                uint64_t entrySize;
            };

            auto buffer = reinterpret_cast<DirectoryEntryInfo*>(arg1);
            buffer->inodeNum = directoryEntry.inodeNum;
            buffer->type = directoryEntry.type;
            buffer->entrySize = directoryEntry.entrySize;

            for (uint64_t i = 0; i < directoryEntry.name.GetLength(); ++i)
                buffer->name[i] = directoryEntry.name[i];
            buffer->name[directoryEntry.name.GetLength()] = '\0';

            return 0;
        }

        default:
            Panic();
    }

    Panic();
}