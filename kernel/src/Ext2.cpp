#include "Ext2.h"
#include <stdint.h>
#include "Memory/Memory.h"
#include "Serial.h"
#include "Panic.h"
#include "Bitmap.h"
#include "String.h"

enum Ext2InodeTypePermissions
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

enum Ext2DirectoryEntryType
{
    DEntryUnknown = 0,
    DEntryRegularFile = 1,
    DEntryDirectory = 2,
    DEntryCharacterDevice = 3,
    DEntryBlockDevice = 4,
    DEntryFIFO = 5,
    DEntrySocket = 6,
    DEntrySymLink = 7
};

const uint32_t INODE_ROOT_DIR = 2;
const uint16_t DEFAULT_FILE_TYPE_PERMISSIONS = RegularFile | UserRead | UserWrite | GroupRead | OtherRead;

Ext2::Ext2(void *_ramDiskAddr) : ramDiskVirtAddr((uint64_t)_ramDiskAddr) { }

void Ext2::Mount(VNode* mountPoint)
{
    Serial::Print("Initializing Ext2Driver file system...");

    superblock = (Ext2Superblock*)(ramDiskVirtAddr + 1024);

    KAssert(superblock->ext2Signature == 0xef53, "Invalid ext2 signature!");
    KAssert(superblock->fileSystemState == 1, "File system state is not clean.");

    // Parse from superblock
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

    blockGroupsCount = superblock->blocksCount / superblock->blocksPerBlockGroup +
                       (superblock->blocksCount % superblock->blocksPerBlockGroup > 0 ? 1 : 0);

    uint32_t blockGroupsCountCheck = superblock->inodesCount / superblock->inodesPerBlockGroup +
                                     (superblock->inodesCount % superblock->inodesPerBlockGroup > 0 ? 1 : 0);

    KAssert(blockGroupsCount == blockGroupsCountCheck, "Block group count could not be calculated.");

    Serial::Printf("Block group count: %d", blockGroupsCount);

    Serial::Printf("First non-reserved rootInode in file system: %d", superblock->firstNonReservedInode);

    Serial::Printf("Optional features: %x", superblock->optionalFeatures);
    Serial::Printf("Required features: %x", superblock->requiredFeatures);
    Serial::Printf("Read-only features: %x", superblock->readOnlyFeatures);

    Serial::Printf("Number of blocks to preallocate for files: %d", superblock->preallocFilesBlocksCount);
    Serial::Printf("Number of blocks to preallocate for directories: %d", superblock->preallocDirectoriesBlocksCount);

    uint32_t blockGroupDescTableDiskAddr = blockSize * (blockSize == 1024 ? 2 : 1);
    blockGroupDescTable = (Ext2BlockGroupDescriptor*)(ramDiskVirtAddr + blockGroupDescTableDiskAddr);

    auto ext2Root = new VNode();
    ext2Root->inodeNum = INODE_ROOT_DIR;
    ext2Root->fileSystem = this;

    Ext2Inode* rootInode = GetInode(INODE_ROOT_DIR);
    ext2Root->context = rootInode;
    ext2Root->fileSize = rootInode->size0;

    mountPoint->mountedVNode = ext2Root;
    CacheVNode(ext2Root);
}

VNode* Ext2::FindInDirectory(VNode* directory, const String& name)
{
    auto context = (Ext2Inode*)directory->context;

    uint64_t parsedLength = 0;
    while (parsedLength < context->size0)
    {
        Ext2DirectoryEntry directoryEntry;
        Read(directory, &directoryEntry, sizeof(Ext2DirectoryEntry), parsedLength);

        if (directoryEntry.inodeNum != 0)
        {
            VNode* child = SearchInCache(directoryEntry.inodeNum, this);

            // SearchInCache returns nullptr if it could not find vnode in cache
            if (child == nullptr)
            {
                child = new VNode();
                child->inodeNum = directoryEntry.inodeNum;
                child->fileSystem = this;

                Ext2Inode* childInode = GetInode(child->inodeNum);
                child->context = childInode;
                child->fileSize = childInode->size0;

                CacheVNode(child);
            }

            char nameBuffer[directoryEntry.nameLength];
            Read(directory, nameBuffer, directoryEntry.nameLength, parsedLength + sizeof(directoryEntry));
            nameBuffer[directoryEntry.nameLength] = 0;

            Serial::Print("------------------- Found: ", "");
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

uint64_t Ext2::Read(VNode* vNode, void* buffer, uint64_t count, uint64_t readPos)
{
    // TODO: Support files larger than 2^32 bytes
    if (readPos + count > vNode->fileSize)
    {
        count = vNode->fileSize - readPos;
    }

    uint64_t currentReadPos = readPos;
    uint64_t parsedCount = 0;

    while (parsedCount < count)
    {
        uint64_t remainingBytes = count - parsedCount;
        uint64_t offsetInBlock = currentReadPos % blockSize;
        uint64_t memCopySize = blockSize - offsetInBlock;
        if (memCopySize > remainingBytes) memCopySize = remainingBytes;

        uint64_t block = GetBlockAddr(vNode, currentReadPos / blockSize, false);
        uint64_t diskAddr = block * blockSize + offsetInBlock;

        // TODO: Read from disk if file system is disk-backed
        MemCopy((void*)((uint64_t)buffer + parsedCount), (void*)(diskAddr + ramDiskVirtAddr), memCopySize);

        parsedCount += memCopySize;
        readPos += memCopySize;
    }

    return parsedCount;
}

uint64_t Ext2::Write(VNode* vNode, const void* buffer, uint64_t count, uint64_t writePos)
{
    uint64_t currentWritePos = writePos;
    uint64_t wroteCount = 0;

    while (wroteCount < count)
    {
        uint64_t remainingBytes = count - wroteCount;
        uint64_t offsetInBlock = currentWritePos % blockSize;
        uint64_t memCopySize = blockSize - offsetInBlock;
        if (memCopySize > remainingBytes) memCopySize = remainingBytes;

        uint64_t block = GetBlockAddr(vNode, currentWritePos / blockSize, true);
        uint64_t diskAddr = block * blockSize + offsetInBlock;

        // TODO: Write to disk if file system is disk-backed
        MemCopy((void*)(diskAddr + ramDiskVirtAddr), (void*)((uint64_t)buffer + wroteCount), memCopySize);

        wroteCount += memCopySize;
        currentWritePos += memCopySize;

        if (vNode->fileSize < currentWritePos)
        {
            // TODO: Write to disk if file system is disk-backed
            auto context = (Ext2Inode*)vNode->context;
            context->size0 = currentWritePos;
            vNode->fileSize = currentWritePos;
        }
    }

    return wroteCount;
}

void Ext2::Create(VNode* vNode, VNode* directory, String name)
{
    auto directoryContext = (Ext2Inode*)directory->context;

    KAssert(directoryContext->typePermissions & 0x4000, "Inode must be a directory to create files in it.");

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
            inodeUsageBitmap.buffer = (uint8_t*)(blockGroup->inodeUsageBitmapBlock * blockSize + ramDiskVirtAddr);
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

    KAssert(inode != nullptr && inodeNum != 0, "Could not create new file, unallocated inode not found.");

    // TODO: Support permissions, file ACL and other fields in ext2 directory
    inode->typePermissions = DEFAULT_FILE_TYPE_PERMISSIONS;
    inode->hardLinksCount = 1;
    inode->size0 = 0;

    uint8_t nameLength = name.GetLength();
    uint8_t nameLengthPadding = 4 - (nameLength % 4);

    Ext2DirectoryEntry directoryEntry;
    directoryEntry.inodeNum = inodeNum;
    directoryEntry.entrySize = (uint16_t)(sizeof(Ext2DirectoryEntry) + nameLength + nameLengthPadding);
    directoryEntry.nameLength = nameLength;
    directoryEntry.typeIndicator = Ext2DirectoryEntryType::DEntryRegularFile;

    Write(directory, &directoryEntry, sizeof(Ext2DirectoryEntry), directory->fileSize);

    // Directory entry name field must be padded so that it's size is a multiple of 4
    Write(directory, name.ToCString(), nameLength, directory->fileSize);

    uint8_t val = 0;
    for (uint8_t i = 0; i < nameLengthPadding; ++i)
    {
        Write(directory, &val, 1, directory->fileSize);
    }

    vNode->context = inode;
    vNode->inodeNum = inodeNum;
    vNode->fileSystem = this;

    vNode->context = inode;
    vNode->fileSize = inode->size0;

    CacheVNode(vNode);
}

uint32_t Ext2::GetBlockAddr(VNode* vNode, uint32_t requestedBlockIndex, bool allocateMissingBlock)
{
    auto context = (Ext2Inode*)vNode->context;

    uint64_t pointersPerBlock = blockSize / sizeof(uint32_t);
    uint32_t block = 0;

    if (requestedBlockIndex < 12)
    {
        block = context->directBlockPointers[requestedBlockIndex];
    }
    else if (requestedBlockIndex < 12 + pointersPerBlock)
    {
        Panic("Singly indirect pointers are not yet supported.");
    }
    else if (requestedBlockIndex < 12 + pointersPerBlock * pointersPerBlock)
    {
        Panic("Doubly indirect pointers are not yet supported.");
    }
    else if (requestedBlockIndex < 12 + pointersPerBlock * pointersPerBlock * pointersPerBlock)
    {
        Panic("Triply indirect pointers are not yet supported.");
    }
    else
    {
        Panic("Unsupported block pointer request (%d).", requestedBlockIndex);
    }

    if (allocateMissingBlock && block == 0)
    {
        // Find unallocated block
        for (uint32_t blockGroupIndex = 0; blockGroupIndex < blockGroupsCount; ++blockGroupIndex)
        {
            Ext2BlockGroupDescriptor* blockGroup = &blockGroupDescTable[blockGroupIndex];
            if (blockGroup->unallocatedBlocksCount > 0)
            {
                uint64_t blockUsageBitmapSize = blockGroup->unallocatedBlocksCount / 8;
                if (blockGroup->unallocatedBlocksCount % 8 != 0) blockUsageBitmapSize++;

                Bitmap blockUsageBitmap;
                blockUsageBitmap.buffer = (uint8_t*)(blockGroup->blockUsageBitmapBlock * blockSize + ramDiskVirtAddr);
                blockUsageBitmap.size = blockUsageBitmapSize;

                for (uint32_t blockIndex = 0; blockIndex < blockUsageBitmap.size * 8; ++blockIndex)
                {
                    if (!blockUsageBitmap.GetBit(blockIndex))
                    {
                        block = blockGroupIndex * superblock->blocksPerBlockGroup + blockIndex;
                        blockUsageBitmap.SetBit(blockIndex, true);
                        blockGroup->unallocatedBlocksCount--;
                        superblock->unallocatedBlocksCount--;
                        context->directBlockPointers[requestedBlockIndex] = block;
                        return block;
                    }
                }
            }
        }
    }

    return block;
}

Ext2Inode* Ext2::GetInode(uint32_t inodeNum)
{
    // Inode numbers start at 1
    uint32_t blockGroupIndex = (inodeNum - 1) / superblock->inodesPerBlockGroup;
    uint32_t inodeIndex = (inodeNum - 1) % superblock->inodesPerBlockGroup;

    uint64_t inodeTableDiskAddr = blockGroupDescTable[blockGroupIndex].inodeTableStartBlock * blockSize;
    uint64_t diskAddr = inodeTableDiskAddr + (inodeIndex * superblock->inodeSize);

    // TODO: Read from disk if the inode is not already cached
    return (Ext2Inode*)(diskAddr + ramDiskVirtAddr);
}