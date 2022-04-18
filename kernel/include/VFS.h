#pragma once

#include "String.h"
#include "Vector.h"
#include "Error.h"

class FileSystem;

class VFS
{
public:
    struct Vnode;
    enum class VnodeType;
    enum class SeekType;

    static VFS* kernelVfs;

    int Open(const String& path, int flags, Error& error);
    int Open(const String& path, int flags);
    uint64_t Read(int descriptor, void* buffer, uint64_t count);
    uint64_t Write(int descriptor, const void* buffer, uint64_t count);
    uint64_t RepositionOffset(int descriptor, uint64_t offset, SeekType seekType, Error& error);
    uint64_t RepositionOffset(int descriptor, uint64_t offset, SeekType seekType);
    void Close(int descriptor);
    VnodeType GetVnodeType(int descriptor, Error& error);
    VnodeType GetVnodeType(int descriptor);

    static void Mount(Vnode* mountPoint, Vnode* vnode);
    static Vnode* CreateDirectory(const String& path, Error& error);
    static Vnode* CreateDirectory(const String& path);

    static void Initialize(void* ext2RamDisk);
    static void CacheVNode(Vnode* vnode);
    static Vnode* SearchInCache(uint32_t inodeNum, FileSystem* fileSystem);
private:
    struct FileDescriptor;
    Vector<FileDescriptor> fileDescriptors;

    FileDescriptor* GetFileDescriptor(int descriptor);
    int FindFreeFileDescriptor(FileDescriptor*& fileDescriptor);

    static Vnode* TraversePath(String path, String& fileName, Vnode*& containingDirectory, FileSystem*& fileSystem, Error& error);
};

enum class VFS::VnodeType
{
    Unknown,
    RegularFile,
    Directory,
    CharacterDevice
};

struct VFS::Vnode
{
    VnodeType type;

    void* context = nullptr;
    uint32_t inodeNum = 0;
    uint32_t fileSize;
    FileSystem* fileSystem;

    Vnode* mountedVNode = nullptr;
    Vnode* nextInCache = nullptr;
};

struct VFS::FileDescriptor
{
    bool present = false;
    uint64_t offset = 0;
    Vnode* vnode = nullptr;
};

enum class VFS::SeekType
{
    Set = 0,
    Cursor = 1,
    End = 2
};
