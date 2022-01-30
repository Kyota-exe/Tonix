#pragma once

#include "String.h"
#include "Vector.h"

enum VnodeType
{
    VFSUnknown,
    VFSRegularFile,
    VFSDirectory,
    VFSCharacterDevice
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
Vnode* CreateDirectory(const String& path);

void CacheVNode(Vnode* vnode);
Vnode* SearchInCache(uint32_t inodeNum, FileSystem* fileSystem);
