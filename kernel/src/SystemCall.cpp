#include "SystemCall.h"
#include "Scheduler.h"
#include "Serial.h"
#include "FileMap.h"

uint64_t SystemCall(SystemCallType type, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, Error& error)
{
    // PLACEHOLDER

    if (type != SystemCallType::Log)
    {
        Serial::Printf("syscall %d", type);
    }

    switch (type)
    {
        case SystemCallType::Open:
            return Open(String((const char*)arg0), (int)arg1, error);

        case SystemCallType::Read:
            return Read((int)arg0, (void*)arg1, arg2);

        case SystemCallType::Write:
            return Write((int)arg0, (const void*)arg1, arg2);

        case SystemCallType::Seek:
            return RepositionOffset((int)arg0, arg1, (VFSSeekType)arg2, error);

        case SystemCallType::Close:
            Close((int)arg0); return 0;

        case SystemCallType::FileMap:
            return (uint64_t)FileMap((void*)arg0, arg1, (int)arg2,(int)arg3, (int)arg4, (int)arg5);

        case SystemCallType::Log:
            Serial::Print((const char*)arg0); return 0;

        case SystemCallType::Panic:
            Panic((const char*)arg0); return 0;

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
            VnodeType vnodeType = GetVnodeType((int)arg0, error);

            // Assuming all VFS character devices are terminals
            if (vnodeType != VnodeType::VFSCharacterDevice)
            {
                error = Error::NotTerminal;
            }

            return error == Error::None ? 0 : -1;
        }

        case SystemCallType::Exit:
            ExitCurrentTask((int)arg0); return 0;

        default:
            Panic("Invalid syscall (%d).", type); return 0;
    }
}