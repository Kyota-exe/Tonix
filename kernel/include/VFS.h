#pragma once

#include "String.h"
#include "Vector.h"
#include "Error.h"

class FileSystem;

class VFS
{
public:
    struct Vnode;
    struct VnodeInfo;
    enum class VnodeType;
    enum OpenFlag : int;
    enum class SeekType;

    static VFS* kernelVfs;

    int Open(const String& path, int flags, Error& error);
    int Open(const String& path, int flags);
    uint64_t Read(int descriptor, void* buffer, uint64_t count, Error& error);
    uint64_t Read(int descriptor, void* buffer, uint64_t count);
    uint64_t Write(int descriptor, const void* buffer, uint64_t count, Error& error);
    uint64_t Write(int descriptor, const void* buffer, uint64_t count);
    uint64_t RepositionOffset(int descriptor, uint64_t offset, SeekType seekType, Error& error);
    uint64_t RepositionOffset(int descriptor, uint64_t offset, SeekType seekType);
    void Close(int descriptor, Error& error);
    void Close(int descriptor);

    VnodeInfo GetVnodeInfo(int descriptor, Error& error);
    VnodeInfo GetVnodeInfo(int descriptor);
    void SetTerminalSettings(int descriptor, bool canonical, bool echo, Error& error);

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
    Unknown = 0,
    RegularFile = 1,
    Directory = 2,
    CharacterDevice = 3
};

struct VFS::Vnode
{
    VnodeType type = VnodeType::Unknown;

    void* context = nullptr;
    uint32_t inodeNum = 0;
    uint32_t fileSize = 0;
    FileSystem* fileSystem = nullptr;

    Vnode* mountedVNode = nullptr;
    Vnode* nextInCache = nullptr;
};

struct VFS::VnodeInfo
{
    VnodeType type;
    uint32_t inodeNum;
    uint32_t fileSize;
} __attribute__((packed));

struct VFS::FileDescriptor
{
    bool present = false;
    uint64_t offset = 0;
    Vnode* vnode = nullptr;
    bool appendMode = false;
    bool readMode = false;
    bool writeMode = false;
};

enum VFS::OpenFlag : int
{
    Create = 0x10,
    Append = 0x8,
    Truncate = 0x200,
    Exclude = 0x40,
    ReadOnly = 0x2,
    WriteOnly = 0x5,
    ReadWrite = 0x3
};

enum class VFS::SeekType
{
    Set = 0,
    Cursor = 1,
    End = 2
};
