#include "VFS.h"
#include "Ext2.h"
#include "DeviceFS.h"
#include "StringUtilities.h"
#include "Vector.h"
#include "Serial.h"
#include "RAMDisk.h"

Vnode* root;
Vnode* currentInCache = nullptr;
VFS* VFS::kernelVfs = nullptr;

void VFS::Initialize(void* ext2RamDisk)
{
    kernelVfs = new VFS();

    root = new Vnode();
    root->type = VFSDirectory;
    currentInCache = root;

    FileSystem* ext2FileSystem;
    ext2FileSystem = new Ext2(new RAMDisk(ext2RamDisk));
    Mount(root, ext2FileSystem->fileSystemRoot);

    Error devMountPointError = Error::None;
    Vnode* devMountPoint = VFS::CreateDirectory(String("/dev"), devMountPointError);
    Assert(devMountPointError == Error::None && devMountPoint != nullptr);

    FileSystem* deviceFileSystem;
    deviceFileSystem = new DeviceFS(nullptr);
    Mount(devMountPoint, deviceFileSystem->fileSystemRoot);
}

void VFS::CacheVNode(Vnode* vnode)
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

Vnode* VFS::TraversePath(String path, String& fileName, Vnode*& containingDirectory, FileSystem*& fileSystem, Error& error)
{
    Vnode* currentDirectory;

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
        Vector<Vnode*> mounts;

        Serial::Print("PATH TOKEN: ", "");
        Serial::Print(fileName);

        // We make a stack of vnodes mounted on this directory so that we can attempt to find the next
        // file from the most recently mounted vnode.
        do
        {
            mounts.Push(currentDirectory);

            if (currentDirectory->type != VFSDirectory)
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

Vnode* VFS::SearchInCache(uint32_t inodeNum, FileSystem* fileSystem)
{
    // VFS root is always first in cache
    Vnode* current = root;

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

void VFS::Mount(Vnode* mountPoint, Vnode* vnode)
{
    Vnode* currentMountPoint = mountPoint;
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

    return descriptorIndex;
}

int VFS::Open(const String& path, int flags, Error& error)
{
    FileDescriptor* fileDescriptor = nullptr;
    int descriptorIndex = FindFreeFileDescriptor(fileDescriptor);

    String filename;
    Vnode* containingDirectory = nullptr;
    FileSystem* fileSystem = nullptr;
    Vnode* vnode = TraversePath(path, filename, containingDirectory, fileSystem, error);

    if (flags & OpenFlag::Create)
    {
        if (error == Error::NoFile)
        {
            Assert(vnode == nullptr);

            vnode = new Vnode();
            vnode->type = VFSRegularFile;
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
        return -1;
    }

    if ((flags & OpenFlag::Truncate) && vnode->type == VFSRegularFile)
    {
        vnode->fileSystem->Truncate(vnode);
    }

    if ((flags & OpenFlag::Append))
    {
        fileDescriptor->offset = vnode->fileSize;
    }

    if (((flags & OpenFlag::WriteOnly) || (flags & OpenFlag::ReadWrite)) && vnode->type == VFSDirectory)
    {
        error = Error::IsDirectory;
        return -1;
    }

    fileDescriptor->vnode = vnode;
    fileDescriptor->present = true;
    return descriptorIndex;
}

uint64_t VFS::Read(int descriptor, void* buffer, uint64_t count)
{
    FileDescriptor* fileDescriptor = GetFileDescriptor(descriptor);
    Assert(fileDescriptor != nullptr);

    Vnode* vnode = fileDescriptor->vnode;

    uint64_t readCount = vnode->fileSystem->Read(vnode, buffer, count, fileDescriptor->offset);
    fileDescriptor->offset += readCount;

    return readCount;
}

uint64_t VFS::Write(int descriptor, const void* buffer, uint64_t count)
{
    FileDescriptor* fileDescriptor = GetFileDescriptor(descriptor);
    Assert(fileDescriptor != nullptr);

    Vnode* vnode = fileDescriptor->vnode;

    uint64_t wroteCount = vnode->fileSystem->Write(vnode, buffer, count, fileDescriptor->offset);
    fileDescriptor->offset += wroteCount;

    return wroteCount;
}

uint64_t VFS::RepositionOffset(int descriptor, uint64_t offset, VFS::SeekType seekType, Error& error)
{
    FileDescriptor* fileDescriptor = GetFileDescriptor(descriptor);
    Assert(fileDescriptor != nullptr);

    Vnode* vnode = fileDescriptor->vnode;

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

    if (fileDescriptor->offset > vnode->fileSize)
    {
        uint8_t val = 0;
        for (uint64_t i = 0; i < fileDescriptor->offset - vnode->fileSize; ++i)
        {
            vnode->fileSystem->Write(vnode, &val, 1, vnode->fileSize);
        }
    }

    return fileDescriptor->offset;
}

void VFS::Close(int descriptor)
{
    FileDescriptor* fileDescriptor = GetFileDescriptor(descriptor);
    Assert(fileDescriptor != nullptr);

    fileDescriptor->present = false;
    fileDescriptor->offset = 0;
}

VnodeType VFS::GetVnodeType(int descriptor, Error& error)
{
    FileDescriptor* fileDescriptor = GetFileDescriptor(descriptor);
    if (fileDescriptor == nullptr)
    {
        error = Error::InvalidFileDescriptor;
        return VnodeType::VFSUnknown;
    }

    return fileDescriptor->vnode->type;
}

Vnode* VFS::CreateDirectory(const String& path, Error& error)
{
    String directoryName;
    Vnode* containingDirectory = nullptr;
    FileSystem* fileSystem = nullptr;
    Vnode* vnode = TraversePath(path, directoryName, containingDirectory, fileSystem, error);

    if (containingDirectory == nullptr)
    {
        return nullptr;
    }

    Assert(vnode == nullptr && error == Error::NoFile);

    vnode = new Vnode();
    vnode->type = VFSDirectory;
    vnode->fileSystem = fileSystem;
    fileSystem->Create(vnode, containingDirectory, directoryName);

    error = Error::None;

    return vnode;
}