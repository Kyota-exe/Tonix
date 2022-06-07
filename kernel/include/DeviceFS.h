#pragma once

#include "VFS.h"
#include "FileSystem.h"
#include "Device.h"

class DeviceFS : public FileSystem
{
public:
    uint64_t Read(VFS::Vnode* vnode, void* buffer, uint64_t count, uint64_t readPos) override;
    uint64_t Write(VFS::Vnode* vnode, const void* buffer, uint64_t count, uint64_t writePos) override;
    VFS::Vnode* FindInDirectory(VFS::Vnode* directory, const String& name) override;
    VFS::DirectoryEntry ReadDirectory(VFS::Vnode* directory, uint64_t readPos) override;
    String GetPathFromSymbolicLink(VFS::Vnode* symLinkVnode) override;
    VFS::Vnode* Create(VFS::Vnode* directory, const String& name, VFS::VnodeType vnodeType) override;
    void Truncate(VFS::Vnode* vnode) override;
    explicit DeviceFS(Disk* disk);
private:
    Vector<Device*> devices;
    uint32_t currentInodeNum = 0;
};
