#include "Ext2.h"
#include <stdint.h>
#include "Serial.h"
#include "Bitmap.h"
#include "String.h"
#include "RAMDisk.h"

enum Ext2InodeTypePermissions : uint16_t
{
    // Types
    FIFO = 0x1000,
    CharacterDevice = 0x2000,
    Directory = 0x4000,
    BlockDevice = 0x6000,
    RegularFile = 0x8000,
    SymbolicLink = 0xa000,
    UnixSocket = 0xc000,

    // Permissions
    UserRead = 0x100,
    UserWrite = 0x080,
    UserExec = 0x040,

    GroupRead = 0x020,
    GroupWrite = 0x010,
    GroupExec = 0x008,

    OtherRead = 0x004,
    OtherWrite = 0x002,
    OtherExec = 0x001,

    SetUserID = 0x800,
    SetGroupID = 0x400,
    StickyBit = 0x200
};

constexpr uint16_t EXT2_SIGNATURE = 0xef53;
constexpr uint64_t EXT2_SUPERBLOCK_DISK_ADDR = 1024;

constexpr uint32_t INODE_ROOT_DIR = 2;
constexpr uint16_t DEFAULT_FILE_TYPE_PERMISSIONS = RegularFile | UserRead | UserWrite | GroupRead | OtherRead;
constexpr uint16_t DEFAULT_DIRECTORY_TYPE_PERMISSIONS = Directory | UserRead | UserWrite | GroupRead | OtherRead;

Ext2::Ext2(Disk* disk) : FileSystem(disk)
{
    Serial::Print("Initializing Ext2 file system...");

    disk->AllocateRead(EXT2_SUPERBLOCK_DISK_ADDR, reinterpret_cast<void**>(&superblock), sizeof(Ext2Superblock));

    Assert(superblock->ext2Signature == EXT2_SIGNATURE);
    Assert(superblock->fileSystemState == 1);

    blockSize = 1024 << superblock->blockSizeLog2Minus10;

    Serial::Printf("Inodes count: %d", superblock->inodesCount);
    Serial::Printf("Inode size: %d", superblock->inodeSize);
    Serial::Printf("Blocks count: %d", superblock->blocksCount);
    Serial::Printf("Block size: %d", blockSize);
    Serial::Printf("Blocks per block group: %d", superblock->blocksPerBlockGroup);
    Serial::Printf("Inodes per block group: %d", superblock->inodesPerBlockGroup);

    Serial::Printf("Version: %d.", superblock->majorVersion, "");
    Serial::Printf("%d", superblock->minorVersion);

    Serial::Print("Volume name: ", "");
    Serial::Print(superblock->volumeName);

    blockGroupsCount = superblock->blocksCount / superblock->blocksPerBlockGroup;
    if (superblock->blocksCount % superblock->blocksPerBlockGroup != 0) blockGroupsCount += 1;

    uint32_t blockGroupsCountCheck = superblock->inodesCount / superblock->inodesPerBlockGroup;
    if (superblock->inodesCount % superblock->inodesPerBlockGroup > 0) blockGroupsCountCheck += 1;

    Assert(blockGroupsCount == blockGroupsCountCheck);

    Serial::Printf("Block group count: %d", blockGroupsCount);

    Serial::Printf("First non-reserved rootInode in file system: %d", superblock->firstNonReservedInode);

    Serial::Printf("Optional features: %x", superblock->optionalFeatures);
    Serial::Printf("Required features: %x", superblock->requiredFeatures);
    Serial::Printf("Read-only features: %x", superblock->readOnlyFeatures);

    Serial::Printf("Number of blocks to preallocate for files: %d", superblock->preallocFilesBlocksCount);
    Serial::Printf("Number of blocks to preallocate for directories: %d", superblock->preallocDirectoriesBlocksCount);

    uint32_t blockGroupDescTableDiskAddr = blockSize * (blockSize == 1024 ? 2 : 1);
    disk->AllocateRead(blockGroupDescTableDiskAddr, reinterpret_cast<void**>(&blockGroupDescTable), sizeof(Ext2BlockGroupDescriptor) * blockGroupsCount);

    fileSystemRoot = new Vnode();
    fileSystemRoot->type = VFSDirectory;
    fileSystemRoot->inodeNum = INODE_ROOT_DIR;
    fileSystemRoot->fileSystem = this;

    Ext2Inode* rootInode = GetInode(INODE_ROOT_DIR);
    fileSystemRoot->context = rootInode;
    fileSystemRoot->fileSize = rootInode->size0;

    VFS::CacheVNode(fileSystemRoot);
}

Vnode* Ext2::CacheDirectoryEntry(Ext2DirectoryEntry* directoryEntry)
{
    auto vnode = new Vnode();
    vnode->inodeNum = directoryEntry->inodeNum;
    vnode->fileSystem = this;

    Ext2Inode* childInode = GetInode(vnode->inodeNum);
    vnode->context = childInode;
    vnode->fileSize = childInode->size0;

    switch (directoryEntry->typeIndicator)
    {
        case DEntryRegularFile:
            vnode->type = VFSRegularFile;
            break;
        case DEntryDirectory:
            vnode->type = VFSDirectory;
            break;
        default:
            vnode->type = VFSUnknown;
    }

    VFS::CacheVNode(vnode);

    return vnode;
}

Vnode* Ext2::FindInDirectory(Vnode* directory, const String& name)
{
    auto context = (Ext2Inode*)directory->context;

    uint64_t parsedLength = 0;
    while (parsedLength < context->size0)
    {
        Ext2DirectoryEntry directoryEntry {};
        Read(directory, &directoryEntry, sizeof(Ext2DirectoryEntry), parsedLength);

        if (directoryEntry.inodeNum != 0)
        {
            Vnode* child = VFS::SearchInCache(directoryEntry.inodeNum, this);

            // SearchInCache returns nullptr if it could not find vnode in cache
            if (child == nullptr)
            {
                child = CacheDirectoryEntry(&directoryEntry);
            }

            char nameBuffer[directoryEntry.nameLength];
            Read(directory, nameBuffer, directoryEntry.nameLength, parsedLength + sizeof(directoryEntry));
            nameBuffer[directoryEntry.nameLength] = 0;

            Serial::Print("[ext2]------------- Found: ", "");
            Serial::Print(nameBuffer);

            if (String(nameBuffer).Equals(name))
            {
                return child;
            }
        }

        parsedLength += directoryEntry.entrySize;
    }

    return nullptr;
}

uint64_t Ext2::Read(Vnode* vnode, void* buffer, uint64_t count, uint64_t readPos)
{
    // TODO: Support files larger than 2^32 bytes
    if (readPos + count > vnode->fileSize)
    {
        count = vnode->fileSize - readPos;
    }

    uint64_t currentReadPos = readPos;
    uint64_t parsedCount = 0;

    while (parsedCount < count)
    {
        uint64_t remainingBytes = count - parsedCount;
        uint64_t offsetInBlock = currentReadPos % blockSize;
        uint64_t readSize = blockSize - offsetInBlock;
        if (readSize > remainingBytes) readSize = remainingBytes;

        uint32_t block = GetBlockAddr(vnode, currentReadPos / blockSize, false);
        uint64_t diskAddr = block * blockSize + offsetInBlock;

        disk->Read(diskAddr, reinterpret_cast<void*>((uintptr_t)buffer + parsedCount), readSize);

        parsedCount += readSize;
        currentReadPos += readSize;
    }

    return parsedCount;
}

uint64_t Ext2::Read(uint32_t block, void* buffer, uint64_t count, uint64_t readPos)
{
    uint64_t diskAddr = block * blockSize + readPos;
    disk->Read(diskAddr, buffer, count);

    return count;
}

uint64_t Ext2::Write(Vnode* vnode, const void* buffer, uint64_t count, uint64_t writePos)
{
    uint64_t currentWritePos = writePos;
    uint64_t wroteCount = 0;

    while (wroteCount < count)
    {
        uint64_t remainingBytes = count - wroteCount;
        uint64_t offsetInBlock = currentWritePos % blockSize;
        uint64_t writeSize = blockSize - offsetInBlock;
        if (writeSize > remainingBytes) writeSize = remainingBytes;

        uint64_t block = GetBlockAddr(vnode, currentWritePos / blockSize, true);
        uint64_t diskAddr = block * blockSize + offsetInBlock;

        disk->Write(diskAddr, reinterpret_cast<const void*>((uintptr_t)buffer + wroteCount), writeSize);

        wroteCount += writeSize;
        currentWritePos += writeSize;

        if (vnode->fileSize < currentWritePos)
        {
            // TODO: Write to disk if file system is disk-backed
            auto context = reinterpret_cast<Ext2Inode*>(vnode->context);
            context->size0 = currentWritePos;
            vnode->fileSize = currentWritePos;
        }
    }

    return wroteCount;
}

void Ext2::Create(Vnode* vnode, Vnode* directory, const String& name)
{
    auto directoryContext = reinterpret_cast<Ext2Inode*>(directory->context);

    Assert(directoryContext->typePermissions & Directory);

    // Find unallocated inode
    Ext2Inode* inode = nullptr;
    uint32_t inodeNum = 0;
    Ext2BlockGroupDescriptor* blockGroup;

    for (uint32_t blockGroupIndex = 0; blockGroupIndex < blockGroupsCount; ++blockGroupIndex)
    {
        blockGroup = &blockGroupDescTable[blockGroupIndex];
        if (blockGroup->unallocatedInodesCount > 0)
        {
            uint64_t inodeUsageBitmapSize = blockGroup->unallocatedInodesCount / 8;
            if (blockGroup->unallocatedInodesCount % 8 != 0) inodeUsageBitmapSize++;

            Bitmap inodeUsageBitmap;
            disk->AllocateRead(blockGroup->inodeUsageBitmapBlock * blockSize, reinterpret_cast<void**>(&inodeUsageBitmap.buffer), inodeUsageBitmapSize);
            inodeUsageBitmap.size = inodeUsageBitmapSize;

            for (uint32_t inodeIndex = 0; inodeIndex < inodeUsageBitmap.size * 8; ++inodeIndex)
            {
                if (!inodeUsageBitmap.GetBit(inodeIndex))
                {
                    inodeNum = blockGroupIndex * superblock->inodesPerBlockGroup + inodeIndex + 1;
                    inode = GetInode(inodeNum);

                    inodeUsageBitmap.SetBit(inodeIndex, true);

                    blockGroup->unallocatedInodesCount--;
                    superblock->unallocatedInodesCount--;

                    goto FoundUnallocatedInode;
                }
            }
        }
    }

    FoundUnallocatedInode:

    Assert(inode != nullptr && inodeNum != 0);

    // TODO: Support file ACL and other fields in ext2 directory
    inode->hardLinksCount = 1;
    inode->size0 = 0;

    switch (vnode->type)
    {
        case VFSRegularFile:
            WriteDirectoryEntry(directory, inodeNum, name, DEntryRegularFile);
            inode->typePermissions = DEFAULT_FILE_TYPE_PERMISSIONS;
            break;
        case VFSDirectory:
            WriteDirectoryEntry(directory, inodeNum, name, DEntryDirectory);
            inode->typePermissions = DEFAULT_DIRECTORY_TYPE_PERMISSIONS;
            break;
        default:
            WriteDirectoryEntry(directory, inodeNum, name, DEntryUnknown);
    }

    vnode->context = inode;
    vnode->inodeNum = inodeNum;
    vnode->fileSystem = this;

    vnode->context = inode;
    vnode->fileSize = inode->size0;

    if (vnode->type == VFSDirectory)
    {
        WriteDirectoryEntry(vnode, vnode->inodeNum, String("."), DEntryDirectory);
        WriteDirectoryEntry(vnode, directory->inodeNum, String(".."), DEntryDirectory);
    }

    VFS::CacheVNode(vnode);
}

void Ext2::Truncate(Vnode* vnode)
{
    auto context = reinterpret_cast<Ext2Inode*>(vnode->context);
    context->size0 = 0;
    vnode->fileSize = 0;
}

void Ext2::WriteDirectoryEntry(Vnode* directory, uint32_t inodeNum, const String& name, Ext2DirectoryEntryType type)
{
    uint8_t nameLength = name.GetLength();
    uint8_t nameLengthPadding = 4 - (nameLength % 4);

    Ext2DirectoryEntry directoryEntry {};
    directoryEntry.typeIndicator = type;
    directoryEntry.inodeNum = inodeNum;
    directoryEntry.entrySize = sizeof(Ext2DirectoryEntry) + nameLength + nameLengthPadding;
    directoryEntry.nameLength = nameLength;

    uint8_t zero = 0;
    Write(directory, &directoryEntry, sizeof(Ext2DirectoryEntry), directory->fileSize);
    Write(directory, name.ToCString(), directoryEntry.nameLength, directory->fileSize);
    for (uint8_t i = 0; i < nameLengthPadding; ++i)
    {
        Write(directory, &zero, 1, directory->fileSize);
    }
}

uint32_t Ext2::GetBlockAddr(Vnode* vnode, uint32_t requestedBlockIndex, bool allocateMissingBlock)
{
    // TODO: Refactor, make recursive

    auto context = reinterpret_cast<Ext2Inode*>(vnode->context);

    uint32_t blockPtr = 0;
    uint64_t pointersPerBlock = blockSize / sizeof(blockPtr);

    if (requestedBlockIndex < 12)
    {
        blockPtr = context->directBlockPointers[requestedBlockIndex];
    }
    else if (requestedBlockIndex < (12 + pointersPerBlock))
    {
        uint64_t readPos = sizeof(blockPtr) * (requestedBlockIndex - 12);
        Read(context->singlyIndirectBlockPtr, &blockPtr, sizeof(blockPtr), readPos);
        return blockPtr;
    }
    else if (requestedBlockIndex < (12 + pointersPerBlock * pointersPerBlock))
    {
        Panic("Doubly indirect pointers are not yet supported.");
    }
    else if (requestedBlockIndex < (12 + pointersPerBlock * pointersPerBlock * pointersPerBlock))
    {
        Panic("Triply indirect pointers are not yet supported.");
    }
    else
    {
        Panic("Unsupported blockPtr pointer request (%d).", requestedBlockIndex);
    }

    if (allocateMissingBlock && blockPtr == 0)
    {
        // Find unallocated blockPtr
        for (uint32_t blockGroupIndex = 0; blockGroupIndex < blockGroupsCount; ++blockGroupIndex)
        {
            Ext2BlockGroupDescriptor* blockGroup = &blockGroupDescTable[blockGroupIndex];
            if (blockGroup->unallocatedBlocksCount > 0)
            {
                uint64_t blockUsageBitmapSize = blockGroup->unallocatedBlocksCount / 8;
                if (blockGroup->unallocatedBlocksCount % 8 != 0) blockUsageBitmapSize++;

                Bitmap blockUsageBitmap;
                disk->AllocateRead(blockGroup->inodeUsageBitmapBlock * blockSize, reinterpret_cast<void**>(&blockUsageBitmap.buffer), blockUsageBitmapSize);
                blockUsageBitmap.size = blockUsageBitmapSize;

                for (uint32_t blockIndex = 0; blockIndex < blockUsageBitmap.size * 8; ++blockIndex)
                {
                    if (!blockUsageBitmap.GetBit(blockIndex))
                    {
                        blockPtr = blockGroupIndex * superblock->blocksPerBlockGroup + blockIndex;
                        blockUsageBitmap.SetBit(blockIndex, true);
                        blockGroup->unallocatedBlocksCount--;
                        superblock->unallocatedBlocksCount--;
                        context->directBlockPointers[requestedBlockIndex] = blockPtr;
                        return blockPtr;
                    }
                }
            }
        }
    }

    return blockPtr;
}

Ext2Inode* Ext2::GetInode(uint32_t inodeNum)
{
    // Inode numbers start at 1
    uint32_t blockGroupIndex = (inodeNum - 1) / superblock->inodesPerBlockGroup;
    uint32_t inodeIndex = (inodeNum - 1) % superblock->inodesPerBlockGroup;

    uint64_t inodeTableDiskAddr = blockGroupDescTable[blockGroupIndex].inodeTableStartBlock * blockSize;
    uint64_t diskAddr = inodeTableDiskAddr + (inodeIndex * superblock->inodeSize);

    Ext2Inode* inode = nullptr;
    disk->AllocateRead(diskAddr, reinterpret_cast<void**>(&inode), sizeof(Ext2Inode));

    return inode;
}