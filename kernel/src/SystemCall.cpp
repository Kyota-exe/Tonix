#include "SystemCall.h"
#include "Scheduler.h"
#include "Serial.h"
#include "FileMap.h"

uint64_t SystemCall(SystemCallType type, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, Error& error)
{
    // PLACEHOLDER

    Serial::Printf("syscall %d", type);

    switch (type)
    {
        case SystemCallType::Open:
            return Open(String((const char*)arg0), (int)arg1, error);

        case SystemCallType::Read:
            return Read((int)arg0, (void*)arg1, arg2);

        case SystemCallType::Write:
            return Write((int)arg0, (const void*)arg1, arg2);

        case SystemCallType::Seek:
            return RepositionOffset((int)arg0, arg1, (VFSSeekType)arg2);

        case SystemCallType::Close:
            Close((int)arg0); return 0;

        case SystemCallType::FileMap:
            return (uint64_t)FileMap((void*)arg0, arg1, (int)arg2,(int)arg3, (int)arg4, (int)arg5);

        case SystemCallType::Log:
            Serial::Print((const char*)arg0); return 0;

        case SystemCallType::Panic:
            Panic((const char*)arg0); return 0;

        default:
            Panic("Invalid syscall (%d).", type); return 0;
    }
}