#pragma once

#include "VFS.h"
#include "FileSystem.h"
#include "Device.h"

class DeviceFS : public FileSystem
{
public:
    uint64_t Read(Vnode* vnode, void* buffer, uint64_t count, uint64_t readPos) override;
    uint64_t Write(Vnode* vnode, const void* buffer, uint64_t count, uint64_t writePos) override;
    Vnode* FindInDirectory(Vnode* directory, const String& name) override;
    void Create(Vnode* vnode, Vnode* directory, const String& name) override;
    void Truncate(Vnode* vnode) override;
    explicit DeviceFS(Disk* disk);
private:
    Vector<Device*> devices;
    uint32_t currentInodeNum = 0;
};