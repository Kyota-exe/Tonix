#include "Device.h"

String Device::GetName() const
{
    return name;
}

uint32_t Device::GetInodeNumber() const
{
    return inodeNum;
}