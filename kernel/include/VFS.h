#pragma once

#include "String.h"
#include "Vector.h"
#include "Error.h"
#include "TerminalDevice.h"
#include "WindowSize.h"

class FileSystem;

class VFS
{
public:
    struct Vnode;
    struct VnodeInfo;
    struct FileDescriptorFlags;
    enum class VnodeType;
    enum OpenFlag : int;
    enum class SeekType;
    struct DirectoryEntry;

    static VFS* kernelVfs;

    int Open(const String& path, int flags, Error& error);
    int Open(const String& path, int flags);
    uint64_t Read(int descriptor, void* buffer, uint64_t count, Error& error);
    void Read(int descriptor, void* buffer, uint64_t count);
    uint64_t Write(int descriptor, const void* buffer, uint64_t count, Error& error);
    void Write(int descriptor, const void* buffer, uint64_t count);
    uint64_t RepositionOffset(int descriptor, int64_t offset, SeekType seekType, Error& error);
    uint64_t RepositionOffset(int descriptor, int64_t offset, SeekType seekType);
    void Close(int descriptor, Error& error);
    void Close(int descriptor);
    void OnExecute();

    VnodeInfo GetVnodeInfo(int descriptor, Error& error);
    VnodeInfo GetVnodeInfo(int descriptor);
    FileDescriptorFlags GetFileDescriptorFlags(int descriptor, Error& error);
    void SetFileDescriptorFlags(int descriptor, const FileDescriptorFlags& flags, Error& error);
    void SetTerminalSettings(int descriptor, bool canonical, bool echo, Error& error);
    WindowSize GetTerminalWindowSize(int descriptor, Error& error);
    String GetWorkingDirectory();
    void SetWorkingDirectory(const String& newWorkingDirectory, Error& error);

    static void Mount(Vnode* mountPoint, Vnode* vnode);
    Vnode* CreateDirectory(const String& path, Error& error);
    Vnode* CreateDirectory(const String& path);

    static void Initialize(void* ext2RamDisk);
    static void CacheVNode(Vnode* vnode);
    static Vnode* ConstructVnode(uint32_t inodeNum, FileSystem* fileSystem, void* context, uint64_t fileSize, VnodeType type);
    static Vnode* SearchInCache(uint32_t inodeNum, FileSystem* fileSystem);

    VFS() = default;
    VFS(const VFS& original);
private:
    struct FileDescriptor;
    struct FileHandle;

    Vector<FileDescriptor> fileDescriptors;
    String workingDirectory = String('/');

    FileDescriptor* GetFileDescriptor(int descriptor);
    int FindFreeFileDescriptor(FileDescriptor*& fileDescriptor);
    TerminalDevice* GetTerminal(int descriptor, Error& error);

    Vnode* TraversePath(String path, String& fileName, Vnode*& containingDirectory, Error& error);
    static String ConvertToAbsolutePath(const String& path, const String& currentDirectoryPath);
};

enum class VFS::VnodeType
{
    Unknown = 0,
    RegularFile = 1,
    Directory = 2,
    Terminal = 3,
    Framebuffer = 4,
    SymbolicLink = 5,
    Keyboard = 6,
    Random = 7,
};

struct VFS::Vnode
{
    VnodeType type = VnodeType::Unknown;

    void* context = nullptr;
    uint32_t inodeNum = 0;
    uint32_t fileSize = 0;
    FileSystem* fileSystem = nullptr;

    Vnode* mountedVnode = nullptr;
    Vnode* nextInCache = nullptr;
};

struct VFS::VnodeInfo
{
    VnodeType type;
    uint32_t inodeNum;
    uint32_t fileSize;
} __attribute__((packed));

struct VFS::FileDescriptorFlags
{
    bool directoryMode = false;
    bool appendMode = false;
    bool readMode = false;
    bool writeMode = false;
    bool closeOnExecute = false;
};

struct VFS::FileHandle
{
    uint64_t refCount = 0;
    Vnode* vnode = nullptr;
    uint64_t offset = 0;
    FileDescriptorFlags flags;
};

struct VFS::FileDescriptor
{
    FileHandle* handle = nullptr;
    bool present = false;
    Vnode*& vnode() { return handle->vnode; };
    uint64_t& offset() { return handle->offset; }
    FileDescriptorFlags& flags() { return handle->flags; };
};

enum VFS::OpenFlag : int
{
    Create = 0x10,
    Append = 0x8,
    Truncate = 0x200,
    Exclude = 0x40,
    ReadOnly = 0x2,
    WriteOnly = 0x5,
    ReadWrite = 0x3,
    DirectoryMode = 0x20,
    CloseOnExecute = 0x4000,
};

enum class VFS::SeekType
{
    Set = 0,
    Cursor = 1,
    End = 2
};

struct VFS::DirectoryEntry
{
    uint32_t inodeNum;
    String name;
    VnodeType type;
    uint64_t entrySize;
};
