#include "VFS.h"
#include "Ext2.h"
#include "StringUtilities.h"
#include "Panic.h"
#include "Vector.h"
#include "Serial.h"
#include "Scheduler.h"

VNode* root;
VNode* currentInCache = nullptr;

void InitializeVFS(void* ext2RamDisk)
{
    FileSystem* ext2FileSystem;
    ext2FileSystem = (FileSystem*)new Ext2(ext2RamDisk);

    root = new VNode();
    currentInCache = root;

    ext2FileSystem->Mount(root);
}

void CacheVNode(VNode* vNode)
{
    currentInCache->nextInCache = vNode;
    currentInCache = vNode;
}

VNode* TraversePath(String path, String& fileName, VNode*& containingDirectory)
{
    VNode* currentDirectory = nullptr;

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
        Vector<VNode*> mounts;

        Serial::Print("PATH TOKEN: ", "");
        Serial::Print(fileName.begin());

        do
        {
            mounts.Push(currentDirectory);
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
                Panic("Could not find currentDirectory.");
            }
            Serial::Print("Could not find file.");

            currentDirectory = new VNode();
            currentDirectory->fileSystem = fileSystem;
            return currentDirectory;
        }
    }

    return currentDirectory;
}

VNode* SearchInCache(uint32_t inodeNum, FileSystem* fileSystem)
{
    // VFS root is always first in cache
    VNode* current = root;

    while (current->nextInCache != nullptr)
    {
        if (current->inodeNum == inodeNum && current->fileSystem == fileSystem)
        {
            return current;
        }

        current = current->nextInCache;
    }

    return nullptr;
}

/*
VNode Create(const char* name, Ext2Driver::Ext2Inode* directoryInode)
{
    uint32_t inodeNum = directoryInode->Create(name);
    return VNode(name, inodeNum);
}
*/

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
    VNode* containingDirectory = nullptr;
    VNode* vNode = TraversePath(path, filename, containingDirectory);
    fileDescriptor->vNode = vNode;

    if ((flags & VFSOpenFlag::OCreate) && fileDescriptor->vNode->inodeNum == 0)
    {
        vNode->fileSystem->Create(vNode, containingDirectory, filename);
    }

    /*
    if ((flags & VFSOpenFlag::OTruncate) && (inode->typePermissions & Ext2Driver::InodeTypePermissions::RegularFile))
    {
        inode->size0 = 0;
    }*/

    if ((flags & VFSOpenFlag::OAppend))
    {
        fileDescriptor->offset = vNode->fileSize;
    }

    return descriptorIndex;
}

uint64_t Read(int descriptor, void* buffer, uint64_t count)
{
    FileDescriptor* fileDescriptor =  &(*taskList)[currentTaskIndex].fileDescriptors[descriptor];
    VNode* vNode = fileDescriptor->vNode;

    uint64_t readCount = vNode->fileSystem->Read(vNode, buffer, count, fileDescriptor->offset);
    fileDescriptor->offset += readCount;

    return readCount;
}

uint64_t Write(int descriptor, const void* buffer, uint64_t count)
{
    FileDescriptor* fileDescriptor = &(*taskList)[currentTaskIndex].fileDescriptors[descriptor];
    VNode* vNode = fileDescriptor->vNode;

    uint64_t wroteCount = vNode->fileSystem->Write(vNode, buffer, count, fileDescriptor->offset);
    fileDescriptor->offset += wroteCount;

    return wroteCount;
}

uint64_t RepositionOffset(int descriptor, uint64_t offset, VFSSeekType seekType)
{
    // TODO: Support files larger than 2^32 bytes
    FileDescriptor* fileDescriptor = &(*taskList)[currentTaskIndex].fileDescriptors[descriptor];
    VNode* vNode = fileDescriptor->vNode;

    switch (seekType)
    {
        case SeekSet:
            fileDescriptor->offset = offset;
            break;
        case SeekCursor:
            fileDescriptor->offset += offset;
            break;
        case SeekEnd:
            fileDescriptor->offset = vNode->fileSize + offset;
            break;
    }

    if (fileDescriptor->offset > vNode->fileSize)
    {
        uint8_t val = 0;
        for (uint64_t i = 0; i < fileDescriptor->offset - vNode->fileSize; ++i)
        {
            vNode->fileSystem->Write(vNode, &val, 1, vNode->fileSize);
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