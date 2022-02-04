#pragma once

struct Vnode;

#include "String.h"
#include "Vector.h"
#include "FileSystem.h"

enum VnodeType
{
    VFSUnknown,
    VFSRegularFile,
    VFSDirectory,
    VFSCharacterDevice
};

enum VFSOpenFlag
{
    OpenCreate = 0x10,
    OpenAppend = 0x8,
    OpenTruncate = 0x200,
    OpenExclude = 0x40,
    OpenWriteOnly = 0x5,
    OpenReadWrite = 0x3
};

enum class VFSError : int
{
    Exists = 17,
    IsDirectory = 21,
    NoFile = 2,
    NotDirectory = 20
};

enum class VFSSeekType
{
    Set = 0,
    Cursor = 1,
    End = 2
};

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
void Mount(Vnode* mountPoint, Vnode* vnode);

int Open(const String& path, int flags);
uint64_t Read(int descriptor, void* buffer, uint64_t count);
uint64_t Write(int descriptor, const void* buffer, uint64_t count);
uint64_t RepositionOffset(int descriptor, uint64_t offset, VFSSeekType seekType);
void Close(int descriptor);
int CreateDirectory(const String& path, Vnode** directory);

void CacheVNode(Vnode* vnode);
Vnode* SearchInCache(uint32_t inodeNum, FileSystem* fileSystem);
