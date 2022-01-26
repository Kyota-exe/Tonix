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

VNode* TraversePath(String path)
{
    VNode* directory = nullptr;

    if (path.Split('/', 0).IsEmpty())
    {
        directory = root;
        path = path.Substring(1, path.GetLength() - 1);
    }
    else Panic("Traversing using relative path is not supported.");

    unsigned int pathDepth = path.Count('/') + 1;

    String pathToken;
    for (unsigned int currentDepth = 0; currentDepth < pathDepth; ++currentDepth)
    {
        pathToken = path.Split('/', currentDepth);
        Vector<VNode*> mounts;

        Serial::Print("PATH TOKEN: ", "");
        Serial::Print(pathToken.begin());

        do
        {
            mounts.Push(directory);
            directory = directory->mountedVNode;
        } while (directory != nullptr);

        do
        {
            directory = mounts.Pop();

            if (directory->fileSystem == nullptr)
            {
                directory = nullptr;
                continue;
            }

            directory = directory->fileSystem->FindInDirectory(directory, pathToken);

        } while (directory == nullptr && mounts.GetLength() > 0);

        KAssert(directory != nullptr, "Could not find directory.");
    }

    return directory;
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
uint64_t RepositionOffset(FileDescriptor* fileDescriptor, Ext2Driver::Ext2Inode* inode, uint64_t offset, VFSSeekType seekType)
{
    switch (seekType)
    {
        case SeekSet:
            fileDescriptor->offset = offset;
            break;
        case SeekCursor:
            fileDescriptor->offset += offset;
            break;
        case SeekEnd:
            fileDescriptor->offset = inode->size0 + offset;
            break;
    }

    if (fileDescriptor->offset > inode->size0)
    {
        inode->Write(0, fileDescriptor->offset - inode->size0, inode->size0, false);
    }

    return fileDescriptor->offset;
}

uint64_t RepositionOffset(int descriptor, uint64_t offset, VFSSeekType seekType)
{
    // TODO: Support files larger than 2^32 bytes
    FileDescriptor* fileDescriptor = &process->fileDescriptors[descriptor];
    Ext2Driver::Ext2Inode* inode = Ext2Driver::GetInode(fileDescriptor->vNode->inodeNum);
    return RepositionOffset(fileDescriptor, inode, offset, seekType);
}

VNode Create(const char* name, Ext2Driver::Ext2Inode* directoryInode)
{
    uint32_t inodeNum = directoryInode->Create(name);
    return VNode(name, inodeNum);
}

void Close(int descriptor)
{
    FileDescriptor* fileDescriptor = &(*taskList)[currentTaskIndex].fileDescriptors[descriptor];
    fileDescriptor->present = false;
    fileDescriptor->offset = 0;
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

    fileDescriptors->Get(descriptorIndex).vNode = TraversePath(path);
    Serial::Printf("FOUND!!!!! --> %x", (uint64_t)fileDescriptors->Get(descriptorIndex).vNode);

    /*Ext2Driver::Ext2Inode* inode = Ext2Driver::GetInode(descriptor->vNode->inodeNum);

    if ((flags & VFSOpenFlag::OCreate) && !exists)
    {
        *descriptor->vNode = Create(filename, inode);
    }

    if ((flags & VFSOpenFlag::OTruncate) && (inode->typePermissions & Ext2Driver::InodeTypePermissions::RegularFile))
    {
        inode->size0 = 0;
    }

    if ((flags & VFSOpenFlag::OAppend))
    {
        RepositionOffset(descriptor, inode, 0, VFSSeekType::SeekEnd);
    }*/

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

    auto inode = (Ext2Inode*)(vNode->context);
    fileDescriptor->offset = inode->size0;

    uint64_t wroteCount = vNode->fileSystem->Write(vNode, buffer, count, fileDescriptor->offset);

    fileDescriptor->offset = 0;

    return wroteCount;
}