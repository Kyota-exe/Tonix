#pragma once

#include "String.h"

class Device
{
public:
    virtual uint64_t Read(void* buffer, uint64_t count) = 0;
    virtual uint64_t Write(const void* buffer, uint64_t count) = 0;
    Device(const String& name, uint32_t inodeNum);

public:
    String name;
    uint32_t inodeNum;
};