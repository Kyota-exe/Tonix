#include "KeyboardDevice.h"

KeyboardDevice* KeyboardDevice::instance = nullptr;

constexpr uint64_t BUFFER_SIZE = 64;
uint8_t ringBuffer[BUFFER_SIZE];
uint64_t readIndex = 0;
uint64_t writeIndex = 0;

uint8_t GetFromBuffer()
{
    if (readIndex >= writeIndex)
    {
        Assert(readIndex == writeIndex);
        return 0;
    }

    uint8_t item = ringBuffer[readIndex % BUFFER_SIZE];
    readIndex++;
    return item;
}

void PushToBuffer(uint8_t scanCode)
{
    ringBuffer[writeIndex % BUFFER_SIZE] = scanCode;
    writeIndex++;
}

uint64_t KeyboardDevice::Read(void* buffer, uint64_t count, uint64_t position)
{
    // Temporarily solution for DOOM to work
    static bool flushed = false;
    if (!flushed)
    {
        readIndex = 0;
        writeIndex = 0;
        flushed = true;
    }

    Assert(count == 1);
    Assert(position == 0);

    uint8_t scanCode = GetFromBuffer();
    if (scanCode != 0)
    {
        *static_cast<uint8_t*>(buffer) = scanCode;
        return count;
    }

    return 0;
}

uint64_t KeyboardDevice::Write(const void* buffer, uint64_t count, uint64_t position)
{
    Assert(count == 1);
    Assert(position == 0);

    uint8_t scanCode = *static_cast<const uint8_t*>(buffer);
    PushToBuffer(scanCode);

    return count;
}

KeyboardDevice::KeyboardDevice(const String& name, uint32_t inodeNum) : Device(name, inodeNum)
{
    Assert(instance == nullptr);
    instance = this;
}
