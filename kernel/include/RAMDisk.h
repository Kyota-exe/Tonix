#pragma once

#include "Disk.h"

class RAMDisk : public Disk
{
public:
    uint64_t Read(uint64_t addr, void* buffer, uint64_t count, bool preAllocated) override;
    uint64_t Write(uint64_t addr, const void* buffer, uint64_t count) override;
    explicit RAMDisk(void* ramDiskAddr);
private:
    uint64_t ramDiskVirtAddr;
};