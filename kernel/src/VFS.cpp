#include "VFS.h"
#include "Ext2.h"
#include "DeviceFS.h"
#include "StringUtilities.h"
#include "Panic.h"
#include "Vector.h"
#include "Serial.h"
#include "Scheduler.h"
#include "RAMDisk.h"

Vnode* root;
Vnode* currentInCache = nullptr;

bool operator&(VFSOpenFlag lhs, VFSOpenFlag rhs) {
    return (
            static_cast<uint16_t>(lhs) &
            static_cast<uint16_t>(rhs)
    );
}

void InitializeVFS(void* ext2RamDisk)
{
    root = new Vnode();
    root->type = VFSDirectory;
    currentInCache = root;

    FileSystem* ext2FileSystem;
    ext2FileSystem = new Ext2(new RAMDisk(ext2RamDisk));
    Mount(root, ext2FileSystem->fileSystemRoot);

    Vnode* devMountPoint = nullptr;
    CreateDirectory(String("/dev"), &devMountPoint);
    FileSystem* deviceFileSystem;
    deviceFileSystem = new DeviceFS(nullptr);
    Mount(devMountPoint, deviceFileSystem->fileSystemRoot);
}

void CacheVNode(Vnode* vnode)
{
    currentInCache->nextInCache = vnode;
    currentInCache = vnode;
}

Vnode* TraversePath(String path, String& fileName, Vnode*& containingDirectory, FileSystem*& fileSystem, int& error)
{
    Vnode* currentDirectory = nullptr;

    if (path.Split('/', 0).IsEmpty())
    {
        currentDirectory = root;
        path = path.Substring(1, path.GetLength() - 1);
    }
    else Panic("Traversing using relative path is not supported.");

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
                error = (int)VFSError::NotDirectory;
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
            if (currentDepth + 1 != pathDepth)
            {
                Panic("Could not find directory.");
            }
            error = (int)VFSError::NoFile;
            return nullptr;
        }
    }

    return currentDirectory;
}

Vnode* SearchInCache(uint32_t inodeNum, FileSystem* fileSystem)
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

void Mount(Vnode* mountPoint, Vnode* vnode)
{
    Vnode* currentMountPoint = mountPoint;
    while (currentMountPoint->mountedVNode != nullptr)
    {
        currentMountPoint = currentMountPoint->mountedVNode;
    }
    currentMountPoint->mountedVNode = vnode;
}

int FindFreeFileDescriptor(FileDescriptor*& fileDescriptor)
{
    Vector<FileDescriptor>* fileDescriptors = &(*taskList)[currentTaskIndex].fileDescriptors;
    int descriptorIndex = -1;

    for (int i = 0; (uint64_t)i < fileDescriptors->GetLength(); ++i)
    {
        if (!fileDescriptors->Get(i).present)
        {
            descriptorIndex = i;
        }
    }

    if (descriptorIndex == -1)
    {
        descriptorIndex = (int)fileDescriptors->GetLength();
        fileDescriptors->Push({false, 0, nullptr});
    }

    fileDescriptor = &fileDescriptors->Get(descriptorIndex);

    return descriptorIndex;
}

int Open(const String& path, int flags)
{
    FileDescriptor* fileDescriptor = nullptr;
    int descriptorIndex = FindFreeFileDescriptor(fileDescriptor);

    String filename;
    Vnode* containingDirectory = nullptr;
    FileSystem* fileSystem = nullptr;
    int error = 0;
    Vnode* vnode = TraversePath(path, filename, containingDirectory, fileSystem, error);

    if (error != 0 && error != (int)VFSError::NoFile)
    {
        return error;
    }

    if (flags & VFSOpenFlag::OpenCreate)
    {
        if (vnode == nullptr)
        {
            vnode = new Vnode();
            vnode->type = VFSRegularFile;
            vnode->fileSystem = fileSystem;
            fileSystem->Create(vnode, containingDirectory, filename);
            error = 0;
        }
        else if (flags & VFSOpenFlag::OpenExclude)
        {
            return (int)VFSError::Exists;
        }
    }

    if (vnode == nullptr)
    {
        KAssert(error == (int)VFSError::NoFile, "Invalid error code returned from traversing.");
        return error;
    }

    if ((flags & VFSOpenFlag::OpenTruncate) && vnode->type == VFSRegularFile)
    {
        vnode->fileSystem->Truncate(vnode);
    }

    if ((flags & VFSOpenFlag::OpenAppend))
    {
        fileDescriptor->offset = vnode->fileSize;
    }

    if (((flags & VFSOpenFlag::OpenWriteOnly) || (flags & VFSOpenFlag::OpenReadWrite)) && vnode->type == VFSDirectory)
    {
        return (int)VFSError::IsDirectory;
    }

    fileDescriptor->vnode = vnode;
    fileDescriptor->present = true;
    return descriptorIndex;
}

uint64_t Read(int descriptor, void* buffer, uint64_t count)
{
    FileDescriptor* fileDescriptor =  &(*taskList)[currentTaskIndex].fileDescriptors[descriptor];
    Vnode* vnode = fileDescriptor->vnode;

    uint64_t readCount = vnode->fileSystem->Read(vnode, buffer, count, fileDescriptor->offset);
    fileDescriptor->offset += readCount;

    return readCount;
}

uint64_t Write(int descriptor, const void* buffer, uint64_t count)
{
    FileDescriptor* fileDescriptor = &(*taskList)[currentTaskIndex].fileDescriptors[descriptor];
    Vnode* vnode = fileDescriptor->vnode;

    uint64_t wroteCount = vnode->fileSystem->Write(vnode, buffer, count, fileDescriptor->offset);
    fileDescriptor->offset += wroteCount;

    return wroteCount;
}

uint64_t RepositionOffset(int descriptor, uint64_t offset, VFSSeekType seekType)
{
    // TODO: Support files larger than 2^32 bytes
    FileDescriptor* fileDescriptor = &(*taskList)[currentTaskIndex].fileDescriptors[descriptor];
    Vnode* vnode = fileDescriptor->vnode;

    switch (seekType)
    {
        case VFSSeekType::Set:
            fileDescriptor->offset = offset;
            break;
        case VFSSeekType::Cursor:
            fileDescriptor->offset += offset;
            break;
        case VFSSeekType::End:
            fileDescriptor->offset = vnode->fileSize + offset;
            break;
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

void Close(int descriptor)
{
    FileDescriptor* fileDescriptor = &(*taskList)[currentTaskIndex].fileDescriptors[descriptor];

    fileDescriptor->present = false;
    fileDescriptor->offset = 0;
}

int CreateDirectory(const String& path, Vnode** directory)
{
    String directoryName;
    Vnode* containingDirectory = nullptr;
    FileSystem* fileSystem = nullptr;
    int error = 0;
    Vnode* vnode = TraversePath(path, directoryName, containingDirectory, fileSystem, error);

    if (error != 0 && error != (int)VFSError::NoFile)
    {
        return error;
    }

    KAssert(vnode == nullptr, "Directory already exists.");

    vnode = new Vnode();
    vnode->type = VFSDirectory;
    vnode->fileSystem = fileSystem;
    fileSystem->Create(vnode, containingDirectory, directoryName);

    if (directory != nullptr) *directory = vnode;

    return 0;
}