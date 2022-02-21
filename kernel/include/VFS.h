#pragma once

struct Vnode;

#include "String.h"
#include "Vector.h"
#include "FileSystem.h"
#include "Error.h"

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

enum class VFSSeekType
{
    Set = 3,
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

int Open(const String& path, int flags, Error& error);
uint64_t Read(int descriptor, void* buffer, uint64_t count);
uint64_t Write(int descriptor, const void* buffer, uint64_t count);
uint64_t RepositionOffset(int descriptor, uint64_t offset, VFSSeekType seekType, Error& error);
void Close(int descriptor);
uint64_t CurrentOffset(int descriptor);
int CreateDirectory(const String& path, Vnode** directory, Error& error);
VnodeType GetVnodeType(int descriptor, Error& error);

void CacheVNode(Vnode* vnode);
Vnode* SearchInCache(uint32_t inodeNum, FileSystem* fileSystem);
