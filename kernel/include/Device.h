#pragma once

#include "String.h"

class Device
{
public:
    virtual uint64_t Read(void* buffer, uint64_t count) = 0;
    virtual uint64_t Write(const void* buffer, uint64_t count) = 0;
    Device(String* _name, uint32_t _inodeNum) : name(_name), inodeNum(_inodeNum) { }

public:
    String* name;
    uint32_t inodeNum;
};