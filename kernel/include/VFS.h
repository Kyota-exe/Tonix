#pragma once

#include "String.h"
#include "Vector.h"

enum VnodeType
{
    VFSUnknown,
    VFSRegularFile,
    VFSDirectory
};

enum VFSOpenFlag
{
    OpenCreate = 0x200,
    OpenAppend = 0x08,
    OpenTruncate = 0x1000
};

enum VFSSeekType
{
    SeekSet = 0,
    SeekCursor = 1,
    SeekEnd = 2
};

class FileSystem;

struct Vnode
{
    VnodeType type;

    void* context = nullptr;
    uint32_t inodeNum = 0;
    uint32_t fileSize;
    FileSystem* fileSystem;

    Vnode* mountedVNode = nullptr;
    Vnode* nextInCache = nullptr;
};

class FileSystem
{
public:
    virtual void Mount(Vnode* mountPoint) = 0;
    virtual uint64_t Read(Vnode* vnode, void* buffer, uint64_t count, uint64_t readPos) = 0;
    virtual uint64_t Write(Vnode* vnode, const void* buffer, uint64_t count, uint64_t writePos) = 0;
    virtual Vnode* FindInDirectory(Vnode* directory, const String& name) = 0;
    virtual void Create(Vnode* vnode, Vnode* directory, const String& name) = 0;
    virtual void Truncate(Vnode* vnode) = 0;
};

struct FileDescriptor
{
    bool present = false;
    uint64_t offset = 0;
    Vnode* vnode;
};

void InitializeVFS(void* ext2RamDisk);

int Open(const String& path, int flags);
uint64_t Read(int descriptor, void* buffer, uint64_t count);
uint64_t Write(int descriptor, const void* buffer, uint64_t count);
uint64_t RepositionOffset(int descriptor, uint64_t offset, VFSSeekType seekType);
void Close(int descriptor);
void CreateDirectory(const String& path);

void CacheVNode(Vnode* vnode);
Vnode* SearchInCache(uint32_t inodeNum, FileSystem* fileSystem);
