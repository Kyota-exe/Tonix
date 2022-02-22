#pragma once

class FileSystem;

#include "VFS.h"
#include "Disk.h"

class FileSystem
{
public:
    virtual uint64_t Read(Vnode* vnode, void* buffer, uint64_t count, uint64_t readPos) = 0;
    virtual uint64_t Write(Vnode* vnode, const void* buffer, uint64_t count, uint64_t writePos) = 0;
    virtual Vnode* FindInDirectory(Vnode* directory, const String& name) = 0;
    virtual void Create(Vnode* vnode, Vnode* directory, const String& name) = 0;
    virtual void Truncate(Vnode* vnode) = 0;
    explicit FileSystem(Disk* disk);
    virtual ~FileSystem();
    FileSystem& operator=(const FileSystem&) = delete;
    Vnode* fileSystemRoot;
protected:
    Disk* disk;
};