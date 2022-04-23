#include "VFS.h"
#include "Ext2.h"
#include "DeviceFS.h"
#include "Vector.h"
#include "Serial.h"
#include "RAMDisk.h"
#include "Heap.h"

VFS::Vnode* root;
VFS::Vnode* currentInCache = nullptr;
VFS* VFS::kernelVfs = nullptr;

void VFS::Initialize(void* ext2RamDisk)
{
    kernelVfs = new VFS();

    root = new (Allocator::Permanent) VFS::Vnode();
    root->type = VFS::VnodeType::Directory;
    currentInCache = root;

    FileSystem* ext2FileSystem;
    ext2FileSystem = new Ext2(new RAMDisk(ext2RamDisk));
    Mount(root, ext2FileSystem->fileSystemRoot);

    VFS::Vnode* devMountPoint = VFS::CreateDirectory(String("/dev"));

    FileSystem* deviceFileSystem;
    deviceFileSystem = new DeviceFS(nullptr);
    Mount(devMountPoint, deviceFileSystem->fileSystemRoot);
}

void VFS::CacheVNode(VFS::Vnode* vnode)
{
    currentInCache->nextInCache = vnode;
    currentInCache = vnode;
}

VFS::FileDescriptor* VFS::GetFileDescriptor(int descriptor)
{
    if (descriptor >= (int)fileDescriptors.GetLength())
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

VFS::Vnode* VFS::TraversePath(String path, String& fileName, VFS::Vnode*& containingDirectory, FileSystem*& fileSystem, Error& error)
{
    VFS::Vnode* currentDirectory;

    if (path.Split('/', 0).IsEmpty())
    {
        currentDirectory = root;
        path = path.Substring(1, path.GetLength() - 1);
    }
    else Panic();

    unsigned int pathDepth = path.Count('/') + 1;

    for (unsigned int currentDepth = 0; currentDepth < pathDepth; ++currentDepth)
    {
        fileName = path.Split('/', currentDepth);
        Vector<VFS::Vnode*> mounts;

        Serial::Print("PATH TOKEN: ", "");
        Serial::Print(fileName);

        // We make a stack of vnodes mounted on this directory so that we can attempt to find the next
        // file from the most recently mounted vnode.
        do
        {
            mounts.Push(currentDirectory);

            if (currentDirectory->type != VFS::VnodeType::Directory)
            {
                error = Error::NotDirectory;
                return nullptr;
            }

            currentDirectory = currentDirectory->mountedVNode;
        } while (currentDirectory != nullptr);

        do
        {
            currentDirectory = mounts.Pop();

            if (currentDirectory->fileSystem == nullptr)
            {
                currentDirectory = nullptr;
                continue;
            }

            fileSystem = currentDirectory->fileSystem;
            containingDirectory = currentDirectory;

            currentDirectory = fileSystem->FindInDirectory(containingDirectory, fileName);
        } while (currentDirectory == nullptr && mounts.GetLength() > 0);

        if (currentDirectory == nullptr)
        {
            error = Error::NoFile;
            return nullptr;
        }
    }

    return currentDirectory;
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
    while (currentMountPoint->mountedVNode != nullptr)
    {
        currentMountPoint = currentMountPoint->mountedVNode;
    }
    currentMountPoint->mountedVNode = vnode;
}

int VFS::FindFreeFileDescriptor(FileDescriptor*& fileDescriptor)
{
    int descriptorIndex = -1;

    for (int i = 0; (uint64_t)i < fileDescriptors.GetLength(); ++i)
    {
        if (!fileDescriptors.Get(i).present)
        {
            descriptorIndex = i;
        }
    }

    if (descriptorIndex == -1)
    {
        descriptorIndex = (int)fileDescriptors.GetLength();
        fileDescriptors.Push({false, 0, nullptr});
    }

    fileDescriptor = &fileDescriptors.Get(descriptorIndex);
    fileDescriptor->offset = 0;
    fileDescriptor->vnode = nullptr;

    return descriptorIndex;
}

int VFS::Open(const String& path, int flags, Error& error)
{
    FileDescriptor* fileDescriptor = nullptr;
    int descriptorIndex = FindFreeFileDescriptor(fileDescriptor);
    Assert(!fileDescriptor->present);

    String filename;
    VFS::Vnode* containingDirectory = nullptr;
    FileSystem* fileSystem = nullptr;
    VFS::Vnode* vnode = TraversePath(path, filename, containingDirectory, fileSystem, error);

    Assert(!filename.IsEmpty());

    if (flags & OpenFlag::Create)
    {
        if (error == Error::NoFile)
        {
            Assert(vnode == nullptr);

            vnode = new (Allocator::Permanent) VFS::Vnode();
            vnode->type = VFS::VnodeType::RegularFile;
            vnode->fileSystem = fileSystem;
            fileSystem->Create(vnode, containingDirectory, filename);
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

    if (flags & OpenFlag::WriteOnly)
    {
        fileDescriptor->readMode = false;
        fileDescriptor->writeMode = true;
    }
    else if (flags & OpenFlag::ReadWrite)
    {
        fileDescriptor->readMode = true;
        fileDescriptor->writeMode = true;
    }
    else
    {
        fileDescriptor->readMode = true;
        fileDescriptor->writeMode = false;
    }

    bool writingAllowed = (flags & OpenFlag::WriteOnly) || (flags & OpenFlag::ReadWrite);

    if (writingAllowed && (flags & OpenFlag::Truncate) && vnode->type == VFS::VnodeType::RegularFile)
    {
        vnode->fileSystem->Truncate(vnode);
    }

    if ((flags & OpenFlag::Append))
    {
        fileDescriptor->appendMode = true;
    }

    if (writingAllowed && vnode->type == VFS::VnodeType::Directory)
    {
        error = Error::IsDirectory;
        return -1;
    }

    fileDescriptor->vnode = vnode;
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

uint64_t VFS::Read(int descriptor, void* buffer, uint64_t count)
{
    Error error = Error::None;
    uint64_t readCount = Read(descriptor, buffer, count, error);
    Assert(error == Error::None);
    return readCount;
}

uint64_t VFS::Read(int descriptor, void* buffer, uint64_t count, Error& error)
{
    FileDescriptor* fileDescriptor = GetFileDescriptor(descriptor);
    if (fileDescriptor == nullptr || !fileDescriptor->readMode)
    {
        error = Error::BadFileDescriptor;
        return 0;
    }

    VFS::Vnode* vnode = fileDescriptor->vnode;

    if (vnode->type == VnodeType::Directory)
    {
        error = Error::IsDirectory;
        return 0;
    }

    uint64_t readCount = vnode->fileSystem->Read(vnode, buffer, count, fileDescriptor->offset);
    fileDescriptor->offset += readCount;

    return readCount;
}

uint64_t VFS::Write(int descriptor, const void* buffer, uint64_t count)
{
    Error error = Error::None;
    uint64_t wroteCount = Write(descriptor, buffer, count, error);
    Assert(error == Error::None);
    return wroteCount;
}

uint64_t VFS::Write(int descriptor, const void* buffer, uint64_t count, Error& error)
{
    FileDescriptor* fileDescriptor = GetFileDescriptor(descriptor);
    if (fileDescriptor == nullptr || !fileDescriptor->writeMode)
    {
        error = Error::BadFileDescriptor;
        return 0;
    }

    VFS::Vnode* vnode = fileDescriptor->vnode;

    if (fileDescriptor->appendMode)
    {
        fileDescriptor->offset = fileDescriptor->vnode->fileSize;
    }

    if (fileDescriptor->offset > vnode->fileSize)
    {
        uint8_t null = 0;
        auto originalFileSize = vnode->fileSize;
        for (uint64_t i = 0; i < fileDescriptor->offset - originalFileSize; ++i)
        {
            vnode->fileSystem->Write(vnode, &null, 1, originalFileSize + i - 1);
        }

        Assert(vnode->fileSize == fileDescriptor->offset);
    }

    uint64_t wroteCount = vnode->fileSystem->Write(vnode, buffer, count, fileDescriptor->offset);
    fileDescriptor->offset += wroteCount;

    return wroteCount;
}

uint64_t VFS::RepositionOffset(int descriptor, uint64_t offset, VFS::SeekType seekType, Error& error)
{
    FileDescriptor* fileDescriptor = GetFileDescriptor(descriptor);
    if (fileDescriptor == nullptr)
    {
        error = Error::InvalidFileDescriptor;
        return -1;
    }

    VFS::Vnode* vnode = fileDescriptor->vnode;

    if (vnode->type == VFS::VnodeType::CharacterDevice)
    {
        error = Error::IsPipe;
        return -1;
    }

    switch (seekType)
    {
        case VFS::SeekType::Set:
            fileDescriptor->offset = offset;
            break;
        case VFS::SeekType::Cursor:
            fileDescriptor->offset += offset;
            break;
        case VFS::SeekType::End:
            fileDescriptor->offset = vnode->fileSize + offset;
            break;
        default:
            error = Error::InvalidArgument;
            return -1;
    }

    return fileDescriptor->offset;
}

uint64_t VFS::RepositionOffset(int descriptor, uint64_t offset, VFS::SeekType seekType)
{
    Error error = Error::None;
    auto result = RepositionOffset(descriptor, offset, seekType, error);
    Assert(error == Error::None);
    return result;
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
}

VFS::VnodeInfo VFS::GetVnodeInfo(int descriptor, Error& error)
{
    FileDescriptor* fileDescriptor = GetFileDescriptor(descriptor);
    if (fileDescriptor == nullptr)
    {
        error = Error::InvalidFileDescriptor;
        return {};
    }

    Vnode* vnode = fileDescriptor->vnode;
    return {vnode->type, vnode->inodeNum, vnode->fileSize};
}

VFS::VnodeInfo VFS::GetVnodeInfo(int descriptor)
{
    Error error = Error::None;
    auto result = GetVnodeInfo(descriptor, error);
    Assert(error == Error::None);
    return result;
}

VFS::Vnode* VFS::CreateDirectory(const String& path, Error& error)
{
    String directoryName;
    VFS::Vnode* containingDirectory = nullptr;
    FileSystem* fileSystem = nullptr;
    VFS::Vnode* vnode = TraversePath(path, directoryName, containingDirectory, fileSystem, error);

    if (containingDirectory == nullptr)
    {
        return nullptr;
    }

    Assert(vnode == nullptr && error == Error::NoFile);

    vnode = new (Allocator::Permanent) VFS::Vnode();
    vnode->type = VFS::VnodeType::Directory;
    vnode->fileSystem = fileSystem;
    fileSystem->Create(vnode, containingDirectory, directoryName);

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