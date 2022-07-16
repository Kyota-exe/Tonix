#pragma once

#include "Device.h"

class RandomDevice : public Device
{
public:
    uint64_t Read(void* buffer, uint64_t count, uint64_t position) override;
    uint64_t Write(const void* buffer, uint64_t count, uint64_t position) override;
    RandomDevice(const String& name, uint32_t inodeNum);
private:
    uint64_t seed = 5323;
};
