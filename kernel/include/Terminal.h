#pragma once

#include "Device.h"

class Terminal : public Device
{
public:
    uint64_t Read(void* buffer, uint64_t count) override;
    uint64_t Write(const void* buffer, uint64_t count) override;
    Terminal(const String& _name, uint32_t _inodeNum);
    ~Terminal();
};