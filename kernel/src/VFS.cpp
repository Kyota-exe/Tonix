#include "VFS.h"
#include "Ext2.h"
#include "DeviceFS.h"
#include "StringUtilities.h"
#include "Panic.h"
#include "Vector.h"
#include "Serial.h"
#include "Scheduler.h"

Vnode* root;
Vnode* currentInCache = nullptr;

void InitializeVFS(void* ext2RamDisk)
{
    root = new Vnode();
    root->type = VFSDirectory;
    currentInCache = root;

    FileSystem* ext2FileSystem;
    ext2FileSystem = new Ext2(ext2RamDisk);
    ext2FileSystem->Mount(root);

    Vnode* devMountPoint = CreateDirectory(String("/dev"));
    FileSystem* deviceFileSystem;
    deviceFileSystem = new DeviceFS();
    deviceFileSystem->Mount(devMountPoint);
}

void CacheVNode(Vnode* vnode)
{
    currentInCache->nextInCache = vnode;
    currentInCache = vnode;
}

Vnode* TraversePath(String path, String& fileName, Vnode*& containingDirectory)
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

        do
        {
            mounts.Push(currentDirectory);

            KAssert(currentDirectory->type == VFSDirectory,"Not a directory.", currentDepth);

            currentDirectory = currentDirectory->mountedVNode;
        } while (currentDirectory != nullptr);

        FileSystem* fileSystem = nullptr;

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
            Serial::Print("Could not find file: ", "");
            Serial::Print(fileName);

            currentDirectory = new Vnode();
            currentDirectory->fileSystem = fileSystem;
            return currentDirectory;
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

int Open(const String& path, int flags)
{
    Vector<FileDescriptor>* fileDescriptors = &(*taskList)[currentTaskIndex].fileDescriptors;

    // Find descriptor int
    int descriptorIndex = -1;
    for (int i = 0; (uint64_t)i < fileDescriptors->GetLength(); ++i)
    {
        if (!fileDescriptors->Get(i).present)
        {
            fileDescriptors->Get(i).present = true;
            descriptorIndex = i;
        }
    }
    if (descriptorIndex == -1)
    {
        descriptorIndex = (int)fileDescriptors->GetLength();
        fileDescriptors->Push({true, 0, nullptr});
    }

    FileDescriptor* fileDescriptor = &fileDescriptors->Get(descriptorIndex);

    String filename;
    Vnode* containingDirectory = nullptr;
    Vnode* vnode = TraversePath(path, filename, containingDirectory);
    fileDescriptor->vnode = vnode;

    if (vnode->inodeNum == 0)
    {
        if (flags & VFSOpenFlag::OpenCreate)
        {
            vnode->type = VFSRegularFile;
            vnode->fileSystem->Create(vnode, containingDirectory, filename);
        }
        else
        {
            Panic("Could not find file.");
        }
    }

    if ((flags & VFSOpenFlag::OpenTruncate) && vnode->type == VnodeType::VFSRegularFile)
    {
        vnode->fileSystem->Truncate(vnode);
    }

    if ((flags & VFSOpenFlag::OpenAppend))
    {
        fileDescriptor->offset = vnode->fileSize;
    }

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
        case SeekSet:
            fileDescriptor->offset = offset;
            break;
        case SeekCursor:
            fileDescriptor->offset += offset;
            break;
        case SeekEnd:
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

Vnode* CreateDirectory(const String& path)
{
    String directoryName;
    Vnode* containingDirectory = nullptr;
    Vnode* vnode = TraversePath(path, directoryName, containingDirectory);

    KAssert(vnode->inodeNum == 0, "Directory already exists.");

    vnode->type = VFSDirectory;
    vnode->fileSystem->Create(vnode, containingDirectory, directoryName);

    return vnode;
}