#include "RandomDevice.h"

uint64_t RandomDevice::Read(void* buffer, uint64_t count, uint64_t position)
{
    Assert(position == 0);

    auto data = static_cast<uint8_t*>(buffer);
    for (uint64_t i = 0; i < count; ++i)
    {
        seed = (8253729 * seed + 2396403);
        data[i] = (seed % 32767) & 0xff;
    }

    return count;
}

uint64_t RandomDevice::Write(const void* buffer, uint64_t count, uint64_t position)
{
    Panic();
    (void)buffer;
    (void)count;
    (void)position;
}

RandomDevice::RandomDevice(const String& name, uint32_t inodeNum) : Device(name, inodeNum) {}
