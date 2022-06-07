#pragma once

class FileSystem;

#include "VFS.h"
#include "Disk.h"

class FileSystem
{
public:
    virtual uint64_t Read(VFS::Vnode* vnode, void* buffer, uint64_t count, uint64_t readPos) = 0;
    virtual uint64_t Write(VFS::Vnode* vnode, const void* buffer, uint64_t count, uint64_t writePos) = 0;
    virtual VFS::Vnode* FindInDirectory(VFS::Vnode* directory, const String& name) = 0;
    virtual VFS::DirectoryEntry ReadDirectory(VFS::Vnode* directory, uint64_t readPos) = 0;
    virtual String GetPathFromSymbolicLink(VFS::Vnode* symLinkVnode) = 0;
    virtual VFS::Vnode* Create(VFS::Vnode* directory, const String& name, VFS::VnodeType vnodeType) = 0;
    virtual void Truncate(VFS::Vnode* vnode) = 0;
    explicit FileSystem(Disk* disk);
    virtual ~FileSystem();
    FileSystem& operator=(const FileSystem&) = delete;
    VFS::Vnode* fileSystemRoot;
protected:
    Disk* disk;
};
