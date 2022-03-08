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

class VFS
{
public:
    enum class SeekType
    {
        Set = 0,
        Cursor = 1,
        End = 2
    };

    static VFS* kernelVfs;

    int Open(const String& path, int flags, Error& error);
    uint64_t Read(int descriptor, void* buffer, uint64_t count);
    uint64_t Write(int descriptor, const void* buffer, uint64_t count);
    uint64_t RepositionOffset(int descriptor, uint64_t offset, SeekType seekType, Error& error);
    void Close(int descriptor);
    VnodeType GetVnodeType(int descriptor, Error& error);

    static void Mount(Vnode* mountPoint, Vnode* vnode);
    static Vnode* CreateDirectory(const String& path, Error& error);

    static void Initialize(void* ext2RamDisk);
    static void CacheVNode(Vnode* vnode);
    static Vnode* SearchInCache(uint32_t inodeNum, FileSystem* fileSystem);
private:
    struct FileDescriptor
    {
        bool present = false;
        uint64_t offset = 0;
        Vnode* vnode = nullptr;
    };

    enum OpenFlag
    {
        Create = 0x10,
        Append = 0x8,
        Truncate = 0x200,
        Exclude = 0x40,
        WriteOnly = 0x5,
        ReadWrite = 0x3
    };

    Vector<FileDescriptor> fileDescriptors;

    FileDescriptor* GetFileDescriptor(int descriptor);
    int FindFreeFileDescriptor(FileDescriptor*& fileDescriptor);

    static Vnode* TraversePath(String path, String& fileName, Vnode*& containingDirectory, FileSystem*& fileSystem, Error& error);
};
