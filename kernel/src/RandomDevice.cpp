#include "RandomDevice.h"

uint64_t RandomDevice::Read(void* buffer, uint64_t count, uint64_t position)
{
    Panic();
    (void)buffer;
    (void)count;
    (void)position;
}

uint64_t RandomDevice::Write(const void* buffer, uint64_t count, uint64_t position)
{
    Panic();
    (void)buffer;
    (void)count;
    (void)position;
}

RandomDevice::RandomDevice(const String& name, uint32_t inodeNum) : Device(name, inodeNum) {}
