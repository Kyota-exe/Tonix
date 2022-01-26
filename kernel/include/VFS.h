#pragma once

#include "String.h"
#include "Vector.h"

enum VFSOpenFlag
{
    OCreate = 0x200,
    OAppend = 0x08,
    OTruncate = 0x1000
};

enum VFSSeekType
{
    SeekSet = 0,
    SeekCursor = 1,
    SeekEnd = 2
};

class FileSystem;

struct VNode
{
    void* context;
    uint32_t inodeNum = 0;
    FileSystem* fileSystem;

    VNode* mountedVNode = nullptr;
    VNode* nextInCache = nullptr;
};

class FileSystem
{
public:
    virtual void Mount(VNode* mountPoint) = 0;
    virtual uint64_t Read(VNode* vNode, void* buffer, uint64_t count, uint64_t readPos) = 0;
    virtual uint64_t Write(VNode* vNode, const void* buffer, uint64_t count, uint64_t writePos) = 0;
    virtual VNode* FindInDirectory(VNode* directory, String name) = 0;
    FileSystem() = default;
};

struct FileDescriptor
{
    bool present = false;
    uint64_t offset = 0;
    VNode* vNode;
};

void InitializeVFS(void* ext2RamDisk);

int Open(const String& path, int flags);
uint64_t Read(int descriptor, void* buffer, uint64_t count);
uint64_t Write(int descriptor, const void* buffer, uint64_t count);

void CacheVNode(VNode* vNode);
VNode* SearchInCache(uint32_t inodeNum, FileSystem* fileSystem);
