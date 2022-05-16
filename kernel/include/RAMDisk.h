#pragma once

#include "Disk.h"

class RAMDisk : public Disk
{
public:
    void Read(uint64_t addr, void* buffer, uint64_t count) override;
    void Write(uint64_t addr, const void* buffer, uint64_t count) override;
    explicit RAMDisk(void* ramDiskAddr);
private:
    uint64_t ramDiskVirtAddr;
};