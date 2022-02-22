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
    struct FileDescriptor
    {
        bool present = false;
        uint64_t offset = 0;
        Vnode* vnode;
    };

    enum class SeekType
    {
        Set = 3,
        Cursor = 1,
        End = 2
    };

    static void Initialize(void* ext2RamDisk);
    static void Mount(Vnode* mountPoint, Vnode* vnode);
    static int Open(const String& path, int flags, Error& error);
    static uint64_t Read(int descriptor, void* buffer, uint64_t count);
    static uint64_t Write(int descriptor, const void* buffer, uint64_t count);
    static uint64_t RepositionOffset(int descriptor, uint64_t offset, SeekType seekType, Error& error);
    static void Close(int descriptor);
    static int CreateDirectory(const String& path, Vnode** directory, Error& error);
    static VnodeType GetVnodeType(int descriptor, Error& error);
    static void CacheVNode(Vnode* vnode);
    static Vnode* SearchInCache(uint32_t inodeNum, FileSystem* fileSystem);
private:
    enum OpenFlag
    {
        Create = 0x10,
        Append = 0x8,
        Truncate = 0x200,
        Exclude = 0x40,
        WriteOnly = 0x5,
        ReadWrite = 0x3
    };

    static Vector<FileDescriptor>* GetFileDescriptorsVector();
    static FileDescriptor* GetFileDescriptor(int descriptor);
    static Vnode* TraversePath(String path, String& fileName, Vnode*& containingDirectory, FileSystem*& fileSystem, Error& error);
    static int FindFreeFileDescriptor(FileDescriptor*& fileDescriptor);
};
