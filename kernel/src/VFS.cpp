#include "VFS.h"
#include "Ext2.h"
#include "DeviceFS.h"
#include "Vector.h"
#include "Serial.h"
#include "RAMDisk.h"
#include "Heap.h"
#include "TerminalDevice.h"

VFS::Vnode* root;
VFS::Vnode* currentInCache = nullptr;
VFS* VFS::kernelVfs = nullptr;

VFS::VFS(const VFS& original) :
    fileDescriptors(original.fileDescriptors),
    workingDirectory(original.workingDirectory)
{
    for (FileDescriptor fileDescriptor : fileDescriptors)
    {
        if (!fileDescriptor.present) continue;
        Assert(fileDescriptor.handle != nullptr);
        Assert(fileDescriptor.handle->refCount > 0);
        fileDescriptor.handle->refCount++;
    }
}

void VFS::Initialize(void* ext2RamDisk)
{
    kernelVfs = new VFS();

    FileSystem* ext2FileSystem = new Ext2(new RAMDisk(ext2RamDisk));
    root = ext2FileSystem->fileSystemRoot;
    currentInCache = root;

    VFS::Vnode* devMountPoint = kernelVfs->CreateDirectory(String("/dev"));
    FileSystem* deviceFileSystem = new DeviceFS(nullptr);
    Mount(devMountPoint, deviceFileSystem->fileSystemRoot);
}

void VFS::CacheVNode(VFS::Vnode* vnode)
{
    currentInCache->nextInCache = vnode;
    currentInCache = vnode;
}

VFS::FileDescriptor* VFS::GetFileDescriptor(int descriptor)
{
    if (descriptor >= static_cast<int>(fileDescriptors.GetLength()))
    {
        return nullptr;
    }

    FileDescriptor* fileDescriptor = &fileDescriptors.Get(descriptor);
    if (!fileDescriptor->present)
    {
        return nullptr;
    }

    return fileDescriptor;
}

String VFS::ConvertToAbsolutePath(const String& path, const String& currentDirectoryPath)
{
    String absolutePath = path;

    bool pathIsAbsolute = path.Split('/', 0).IsEmpty();
    if (!pathIsAbsolute)
    {
        Assert(currentDirectoryPath[currentDirectoryPath.GetLength() - 1] == '/');
        absolutePath.Insert(currentDirectoryPath, 0);
    }

    // Remove the trailing '/' if there is one
    if (absolutePath[absolutePath.GetLength() - 1] == '/')
    {
        absolutePath = absolutePath.Substring(0, absolutePath.GetLength() - 1);
    }

    return absolutePath;
}

VFS::Vnode* VFS::TraversePath(String path, String& fileName, VFS::Vnode*& containingDirectory, Error& error)
{
    Assert(!path.IsEmpty());

    path = ConvertToAbsolutePath(path, workingDirectory);
    VFS::Vnode* currentEntry = root;

    uint64_t parsedLength = 1;
    unsigned int pathDepth = path.Count('/');
    for (unsigned int currentDepth = 0; currentDepth < pathDepth; ++currentDepth)
    {
        fileName = path.Split('/', currentDepth + 1);
        Assert(!fileName.IsEmpty());

        Serial::Log("PATH TOKEN: %s", fileName.ToRawString());

        // We make a stack of vnodes mounted on this directory so that we can attempt to find the next
        // file from the most recently mounted vnode.
        Vector<VFS::Vnode*> mounts;
        do
        {
            mounts.Push(currentEntry);

            if (currentEntry->type != VFS::VnodeType::Directory)
            {
                error = Error::NotDirectory;
                return nullptr;
            }

            currentEntry = currentEntry->mountedVnode;
        } while (currentEntry != nullptr);

        do
        {
            currentEntry = mounts.Pop();

            if (currentEntry->fileSystem == nullptr)
            {
                currentEntry = nullptr;
                continue;
            }

            containingDirectory = currentEntry;

            currentEntry = containingDirectory->fileSystem->FindInDirectory(containingDirectory, fileName);
        } while (currentEntry == nullptr && mounts.GetLength() > 0);

        if (currentEntry == nullptr)
        {
            error = Error::NoFile;
            return nullptr;
        }

        if (currentEntry->type == VnodeType::SymbolicLink)
        {
            Assert(currentDepth == pathDepth - 1);

            String symLinkPath = currentEntry->fileSystem->GetPathFromSymbolicLink(currentEntry);
            String currentDirectory = path.Substring(0, parsedLength);
            String symLinkAbsolutePath = ConvertToAbsolutePath(symLinkPath, currentDirectory);
            currentEntry = TraversePath(symLinkAbsolutePath, fileName, containingDirectory, error);
        }

        parsedLength += fileName.GetLength() + 1;
    }

    return currentEntry;
}

VFS::Vnode* VFS::SearchInCache(uint32_t inodeNum, FileSystem* fileSystem)
{
    // VFS root is always first in cache
    VFS::Vnode* current = root;

    while (current != nullptr)
    {
        if (current->inodeNum == inodeNum && current->fileSystem == fileSystem)
        {
            return current;
        }

        current = current->nextInCache;
    }

    return nullptr;
}

void VFS::Mount(VFS::Vnode* mountPoint, VFS::Vnode* vnode)
{
    VFS::Vnode* currentMountPoint = mountPoint;
    while (currentMountPoint->mountedVnode != nullptr)
    {
        currentMountPoint = currentMountPoint->mountedVnode;
    }
    currentMountPoint->mountedVnode = vnode;
}

int VFS::FindFreeFileDescriptor(FileDescriptor*& fileDescriptor)
{
    int descriptorIndex = -1;

    for (uint64_t i = 0; i < fileDescriptors.GetLength(); ++i)
    {
        if (!fileDescriptors.Get(i).present)
        {
            Assert(i <= INT32_MAX);
            descriptorIndex = static_cast<int>(i);
        }
    }

    if (descriptorIndex == -1)
    {
        Assert(fileDescriptors.GetLength() <= INT32_MAX);
        descriptorIndex = static_cast<int>(fileDescriptors.GetLength());
        fileDescriptors.Push({});
    }

    fileDescriptor = &fileDescriptors.Get(descriptorIndex);

    fileDescriptor->handle = new FileHandle;
    fileDescriptor->handle->refCount++;

    fileDescriptor->offset() = 0;
    fileDescriptor->vnode() = nullptr;

    return descriptorIndex;
}

int VFS::Open(const String& path, int flags, Error& error)
{
    FileDescriptor* fileDescriptor = nullptr;
    int descriptorIndex = FindFreeFileDescriptor(fileDescriptor);
    Assert(!fileDescriptor->present);

    String filename;
    VFS::Vnode* containingDirectory = nullptr;
    VFS::Vnode* vnode = TraversePath(path, filename, containingDirectory, error);

    if (flags & OpenFlag::Create)
    {
        if (error == Error::NoFile)
        {
            Assert(vnode == nullptr);
            vnode = containingDirectory->fileSystem->Create(containingDirectory, filename, VFS::VnodeType::RegularFile);
            error = Error::None;
        }
        else if (flags & OpenFlag::Exclude)
        {
            error = Error::Exists;
            return -1;
        }
    }

    if (vnode == nullptr)
    {
        Assert(error == Error::NoFile || error == Error::NotDirectory);
        return -1;
    }

    Assert(error == Error::None);

    int accessBits = flags & 0b111;
    if (accessBits == OpenFlag::WriteOnly)
    {
        fileDescriptor->flags().readMode = false;
        fileDescriptor->flags().writeMode = true;
    }
    else if (accessBits == OpenFlag::ReadWrite)
    {
        fileDescriptor->flags().readMode = true;
        fileDescriptor->flags().writeMode = true;
    }
    else
    {
        fileDescriptor->flags().readMode = true;
        fileDescriptor->flags().writeMode = false;
    }

    if (fileDescriptor->flags().writeMode && (flags & OpenFlag::Truncate))
    {
        Assert(vnode->type == VFS::VnodeType::RegularFile);
        vnode->fileSystem->Truncate(vnode);
    }

    fileDescriptor->flags().appendMode = flags & OpenFlag::Append;
    fileDescriptor->flags().directoryMode = flags & OpenFlag::DirectoryMode;
    fileDescriptor->flags().closeOnExecute = flags & OpenFlag::CloseOnExecute;

    if (fileDescriptor->flags().directoryMode && vnode->type != VnodeType::Directory)
    {
        error = Error::NotDirectory;
        return -1;
    }

    if (fileDescriptor->flags().writeMode && vnode->type == VFS::VnodeType::Directory)
    {
        error = Error::IsDirectory;
        return -1;
    }

    Assert(vnode->type != VnodeType::Unknown);
    fileDescriptor->vnode() = vnode;
    fileDescriptor->present = true;
    return descriptorIndex;
}

int VFS::Open(const String& path, int flags)
{
    Error error = Error::None;
    int desc = Open(path, flags, error);
    Assert(error == Error::None);
    return desc;
}

void VFS::Read(int descriptor, void* buffer, uint64_t count)
{
    Error error = Error::None;
    uint64_t readCount = Read(descriptor, buffer, count, error);
    Assert(error == Error::None);
    Assert(readCount == count);
}

uint64_t VFS::Read(int descriptor, void* buffer, uint64_t count, Error& error)
{
    FileDescriptor* fileDescriptor = GetFileDescriptor(descriptor);
    if (fileDescriptor == nullptr || !fileDescriptor->flags().readMode)
    {
        error = Error::BadFileDescriptor;
        return 0;
    }

    VFS::Vnode* vnode = fileDescriptor->vnode();

    uint64_t readCount = 0;

    if (!fileDescriptor->flags().directoryMode)
    {
        if (vnode->type == VnodeType::Directory)
        {
            error = Error::IsDirectory;
            return -1;
        }

        readCount = vnode->fileSystem->Read(vnode, buffer, count, fileDescriptor->offset());
    }
    else
    {
        Assert(count == sizeof(DirectoryEntry));
        Assert(vnode->type == VnodeType::Directory);

        if (fileDescriptor->offset() < vnode->fileSize)
        {
            auto directoryEntry = static_cast<DirectoryEntry*>(buffer);
            *directoryEntry = vnode->fileSystem->ReadDirectory(vnode, fileDescriptor->offset());
            readCount = directoryEntry->entrySize;
        }
        else
        {
            readCount = 0;
        }
    }

    if (vnode->type == VnodeType::RegularFile || vnode->type == VnodeType::Directory)
    {
        fileDescriptor->offset() += readCount;
    }

    return readCount;
}

void VFS::Write(int descriptor, const void* buffer, uint64_t count)
{
    Error error = Error::None;
    uint64_t wroteCount = Write(descriptor, buffer, count, error);
    Assert(error == Error::None);
    Assert(wroteCount == count);
}

uint64_t VFS::Write(int descriptor, const void* buffer, uint64_t count, Error& error)
{
    FileDescriptor* fileDescriptor = GetFileDescriptor(descriptor);
    if (fileDescriptor == nullptr || !fileDescriptor->flags().writeMode)
    {
        error = Error::BadFileDescriptor;
        return 0;
    }

    VFS::Vnode* vnode = fileDescriptor->vnode();

    if (vnode->type == VnodeType::Directory)
    {
        error = Error::IsDirectory;
        return 0;
    }

    if (fileDescriptor->flags().appendMode)
    {
        fileDescriptor->offset() = fileDescriptor->vnode()->fileSize;
    }

    if (vnode->type == VnodeType::RegularFile && fileDescriptor->offset() > vnode->fileSize)
    {
        uint8_t null = 0;
        auto originalFileSize = vnode->fileSize;
        for (uint64_t i = 0; i < fileDescriptor->offset() - originalFileSize; ++i)
        {
            vnode->fileSystem->Write(vnode, &null, 1, originalFileSize + i);
        }

        Assert(vnode->fileSize == fileDescriptor->offset());
    }

    uint64_t wroteCount = vnode->fileSystem->Write(vnode, buffer, count, fileDescriptor->offset());

    if (vnode->type == VnodeType::RegularFile)
    {
        fileDescriptor->offset() += wroteCount;
    }

    return wroteCount;
}

uint64_t VFS::RepositionOffset(int descriptor, int64_t offset, VFS::SeekType seekType, Error& error)
{
    FileDescriptor* fileDescriptor = GetFileDescriptor(descriptor);
    if (fileDescriptor == nullptr)
    {
        error = Error::InvalidFileDescriptor;
        return -1;
    }

    VFS::Vnode* vnode = fileDescriptor->vnode();

    if (vnode->type == VFS::VnodeType::Terminal)
    {
        error = Error::IsPipe;
        return -1;
    }

    switch (seekType)
    {
        case VFS::SeekType::Set:
            fileDescriptor->offset() = offset;
            break;
        case VFS::SeekType::Cursor:
            fileDescriptor->offset() += offset;
            break;
        case VFS::SeekType::End:
            fileDescriptor->offset() = vnode->fileSize + offset;
            break;
        default:
            error = Error::InvalidArgument;
            return -1;
    }

    return fileDescriptor->offset();
}

void VFS::SetTerminalSettings(int descriptor, bool canonical, bool echo, Error& error)
{
    auto terminal = GetTerminal(descriptor, error);
    if (error != Error::None)
    {
        Assert(terminal == nullptr);
        return;
    }

    Assert(terminal != nullptr);
    terminal->canonical = canonical;
    terminal->echo = echo;
}

WindowSize VFS::GetTerminalWindowSize(int descriptor, Error& error)
{
    auto terminal = GetTerminal(descriptor, error);
    if (error != Error::None)
    {
        Assert(terminal == nullptr);
        return {};
    }

    Assert(terminal != nullptr);
    return terminal->GetWindowSize();
}

TerminalDevice* VFS::GetTerminal(int descriptor, Error& error)
{
    VnodeInfo vnodeInfo = GetVnodeInfo(descriptor, error);
    if (error != Error::None) return nullptr;

    if (vnodeInfo.type == VnodeType::Terminal)
    {
        FileDescriptor* fileDescriptor = GetFileDescriptor(descriptor);
        return static_cast<TerminalDevice*>(fileDescriptor->vnode()->context);
    }
    else
    {
        error = Error::NotTerminal;
        return nullptr;
    }
}

uint64_t VFS::RepositionOffset(int descriptor, int64_t offset, SeekType seekType)
{
    Error error = Error::None;
    auto newOffset = RepositionOffset(descriptor, offset, seekType, error);
    Assert(error == Error::None);
    return newOffset;
}

void VFS::Close(int descriptor)
{
    Error error = Error::None;
    Close(descriptor, error);
    Assert(error == Error::None);
}

void VFS::Close(int descriptor, Error& error)
{
    FileDescriptor* fileDescriptor = GetFileDescriptor(descriptor);
    if (fileDescriptor == nullptr)
    {
        error = Error::InvalidFileDescriptor;
        return;
    }

    fileDescriptor->present = false;

    Assert(fileDescriptor->handle->refCount > 0);
    fileDescriptor->handle->refCount--;
    if (fileDescriptor->handle->refCount == 0)
    {
        delete fileDescriptor->handle;
    }
}

VFS::VnodeInfo VFS::GetVnodeInfo(int descriptor, Error& error)
{
    FileDescriptor* fileDescriptor = GetFileDescriptor(descriptor);
    if (fileDescriptor == nullptr)
    {
        error = Error::InvalidFileDescriptor;
        return {};
    }

    Vnode* vnode = fileDescriptor->vnode();
    return {vnode->type, vnode->inodeNum, vnode->fileSize};
}

VFS::VnodeInfo VFS::GetVnodeInfo(int descriptor)
{
    Error error = Error::None;
    auto result = GetVnodeInfo(descriptor, error);
    Assert(error == Error::None);
    return result;
}

VFS::FileDescriptorFlags VFS::GetFileDescriptorFlags(int descriptor, Error& error)
{
    FileDescriptor* fileDescriptor = GetFileDescriptor(descriptor);
    if (fileDescriptor == nullptr)
    {
        error = Error::InvalidFileDescriptor;
        return {};
    }

    return fileDescriptor->flags();
}

void VFS::SetFileDescriptorFlags(int descriptor, const FileDescriptorFlags& flags, Error& error)
{
    FileDescriptor* fileDescriptor = GetFileDescriptor(descriptor);
    if (fileDescriptor == nullptr)
    {
        error = Error::InvalidFileDescriptor;
        return;
    }

    fileDescriptor->flags() = flags;
}

String VFS::GetWorkingDirectory()
{
    return workingDirectory;
}

void VFS::SetWorkingDirectory(const String& newWorkingDirectory, Error& error)
{
    Assert(!newWorkingDirectory.IsEmpty());

    String filename;
    VFS::Vnode* containingDirectory;
    TraversePath(newWorkingDirectory, filename, containingDirectory, error);

    if (error == Error::None)
    {
        workingDirectory = ConvertToAbsolutePath(newWorkingDirectory, workingDirectory);
        workingDirectory.Push('/');
    }
}

VFS::Vnode* VFS::CreateDirectory(const String& path, Error& error)
{
    String directoryName;
    VFS::Vnode* containingDirectory = nullptr;
    VFS::Vnode* vnode = TraversePath(path, directoryName, containingDirectory, error);

    if (containingDirectory == nullptr) return nullptr;
    Assert(vnode == nullptr);
    Assert(error == Error::NoFile);

    vnode = containingDirectory->fileSystem->Create(containingDirectory, directoryName, VFS::VnodeType::Directory);

    error = Error::None;

    return vnode;
}

VFS::Vnode* VFS::CreateDirectory(const String& path)
{
    Error error = Error::None;
    auto result = CreateDirectory(path, error);
    Assert(error == Error::None);
    return result;
}

VFS::Vnode* VFS::ConstructVnode(uint32_t inodeNum, FileSystem* fileSystem, void* context, uint64_t fileSize, VnodeType type)
{
    auto vnode = new (Allocator::Permanent) VFS::Vnode();
    vnode->inodeNum = inodeNum;
    vnode->fileSystem = fileSystem;
    vnode->context = context;
    vnode->fileSize = fileSize;
    vnode->type = type;
    VFS::CacheVNode(vnode);

    return vnode;
}

void VFS::OnExecute()
{
    for (uint64_t i = 0; i < fileDescriptors.GetLength(); ++i)
    {
        FileDescriptor& fileDescriptor = fileDescriptors.Get(i);
        if (fileDescriptor.present && fileDescriptor.flags().closeOnExecute)
        {
            Close(static_cast<int>(i));
        }
    }
}
