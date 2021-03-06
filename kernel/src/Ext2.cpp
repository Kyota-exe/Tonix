#include "Ext2.h"
#include <stdint.h>
#include "Serial.h"
#include "Bitmap.h"
#include "String.h"
#include "RAMDisk.h"
#include "Heap.h"
#include "Memory/Memory.h"
#include "Math.h"

enum InodeTypePermissions : uint16_t
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
    Serial::Log("Initializing Ext2 file system...");

    superblock = new Superblock;
    disk->Read(EXT2_SUPERBLOCK_DISK_ADDR, superblock, sizeof(Superblock));

    Assert(superblock->ext2Signature == EXT2_SIGNATURE);
    Assert(superblock->fileSystemState == 1);

    blockSize = 1024 << superblock->blockSizeLog2Minus10;

    Serial::Log("Inodes count: %d", superblock->inodesCount);
    Serial::Log("Inode size: %d", superblock->inodeSize);
    Serial::Log("Blocks count: %d", superblock->blocksCount);
    Serial::Log("Block size: %d", blockSize);
    Serial::Log("Blocks per block group: %d", superblock->blocksPerBlockGroup);
    Serial::Log("Inodes per block group: %d", superblock->inodesPerBlockGroup);

    Serial::Log("Version: %d.%d", superblock->majorVersion, superblock->minorVersion);

    Serial::Log("Volume name: %s", superblock->volumeName);

    blockGroupsCount = superblock->blocksCount / superblock->blocksPerBlockGroup;
    if (superblock->blocksCount % superblock->blocksPerBlockGroup != 0) blockGroupsCount += 1;

    uint32_t blockGroupsCountCheck = superblock->inodesCount / superblock->inodesPerBlockGroup;
    if (superblock->inodesCount % superblock->inodesPerBlockGroup > 0) blockGroupsCountCheck += 1;

    Assert(blockGroupsCount == blockGroupsCountCheck);

    Serial::Log("Block group count: %d", blockGroupsCount);

    Serial::Log("First non-reserved rootInode in file system: %d", superblock->firstNonReservedInode);

    Serial::Log("Optional features: %x", superblock->optionalFeatures);
    Serial::Log("Required features: %x", superblock->requiredFeatures);
    Serial::Log("Read-only features: %x", superblock->readOnlyFeatures);

    Serial::Log("Number of blocks to preallocate for files: %d", superblock->preallocFilesBlocksCount);
    Serial::Log("Number of blocks to preallocate for directories: %d", superblock->preallocDirectoriesBlocksCount);

    blockGroupDescTable = new BlockGroupDescriptor[blockGroupsCount];
    uint32_t blockGroupDescTableDiskAddr = blockSize * (blockSize == 1024 ? 2 : 1);
    disk->Read(blockGroupDescTableDiskAddr, blockGroupDescTable, sizeof(BlockGroupDescriptor) * blockGroupsCount);

    Inode* rootInode = GetInode(INODE_ROOT_DIR);
    Assert(rootInode->size1 == 0);
    fileSystemRoot = VFS::ConstructVnode(INODE_ROOT_DIR, this, rootInode, rootInode->size0, VFS::VnodeType::Directory);
}

VFS::Vnode* Ext2::FindInDirectory(VFS::Vnode* directory, const String& name)
{
    auto context = static_cast<Inode*>(directory->context);
    Assert(context->size1 == 0);

    uint64_t parsedLength = 0;
    while (parsedLength < context->size0)
    {
        VFS::DirectoryEntry directoryEntry = ReadDirectory(directory, parsedLength);
        Assert(directoryEntry.inodeNum != 0);

        VFS::Vnode* child = VFS::SearchInCache(directoryEntry.inodeNum, this);
        if (child == nullptr)
        {
            Inode* childInode = GetInode(directoryEntry.inodeNum);
            Assert(childInode->size1 == 0);
            child = VFS::ConstructVnode(directoryEntry.inodeNum, this, childInode, childInode->size0, directoryEntry.type);
        }

        if (directoryEntry.name.Equals(name))
        {
            return child;
        }

        parsedLength += directoryEntry.entrySize;
    }

    Serial::Log("[ext2]------------- Failed to find %s", name.ToRawString());

    return nullptr;
}

VFS::DirectoryEntry Ext2::ReadDirectory(VFS::Vnode* directory, uint64_t readPos)
{
    Ext2::DirectoryEntry ext2DirectoryEntry {};
    Read(directory, &ext2DirectoryEntry, sizeof(ext2DirectoryEntry), readPos);

    char nameBuffer[ext2DirectoryEntry.nameLength];
    Read(directory, nameBuffer, ext2DirectoryEntry.nameLength, readPos + sizeof(ext2DirectoryEntry));
    nameBuffer[ext2DirectoryEntry.nameLength] = 0;

    VFS::VnodeType type;
    switch (ext2DirectoryEntry.typeIndicator)
    {
        case Ext2::DirectoryEntryType::DEntryRegularFile:
            type = VFS::VnodeType::RegularFile;
            break;
        case Ext2::DirectoryEntryType::DEntryDirectory:
            type = VFS::VnodeType::Directory;
            break;
        case Ext2::DirectoryEntryType::DEntrySymLink:
            type = VFS::VnodeType::SymbolicLink;
            break;
        default:
            type = VFS::VnodeType::Unknown;
    }

    return {
        .inodeNum = ext2DirectoryEntry.inodeNum,
        .name = String(nameBuffer),
        .type = type,
        .entrySize = ext2DirectoryEntry.entrySize,
    };
}

uint64_t Ext2::Read(VFS::Vnode* vnode, void* buffer, uint64_t count, uint64_t readPos)
{
    if (readPos + count > vnode->fileSize)
    {
        count = vnode->fileSize - readPos;
    }
    return DiskOperation(IOType::Read, vnode, buffer, count, readPos);
}

uint64_t Ext2::Read(uint32_t block, void* buffer, uint64_t count, uint64_t readPos)
{
    uint64_t diskAddr = block * blockSize + readPos;
    disk->Read(diskAddr, buffer, count);

    return count;
}

uint64_t Ext2::Write(VFS::Vnode* vnode, const void* buffer, uint64_t count, uint64_t writePos)
{
    uint64_t wroteCount = DiskOperation(IOType::Write, vnode, const_cast<void*>(buffer), count, writePos);
    uint64_t newSize = writePos + wroteCount;
    if (vnode->fileSize < writePos + wroteCount)
    {
        auto context = static_cast<Inode*>(vnode->context);
        Assert(context->size1 == 0);
        context->size0 = newSize;
        vnode->fileSize = newSize;
    }

    return wroteCount;
}

uint64_t Ext2::DiskOperation(IOType ioType, VFS::Vnode* vnode, void* buffer, uint64_t count, uint64_t position)
{
    uint64_t currentPos = position;
    uint64_t completedCount = 0;

    while (completedCount < count)
    {
        uint64_t remainingCount = count - completedCount;
        uint64_t offsetInBlock = currentPos % blockSize;
        uint64_t ioSize = blockSize - offsetInBlock;
        if (ioSize > remainingCount) ioSize = remainingCount;

        bool allocateMissingBlock = ioType == IOType::Write;
        uint64_t block = GetBlockAddr(vnode, currentPos / blockSize, allocateMissingBlock);
        uint64_t diskAddr = block * blockSize + offsetInBlock;
        uintptr_t bufferAddr = reinterpret_cast<uintptr_t>(buffer) + completedCount;

        switch (ioType)
        {
            case IOType::Read:
                disk->Read(diskAddr, reinterpret_cast<void*>(bufferAddr), ioSize);
                break;
            case IOType::Write:
                disk->Write(diskAddr, reinterpret_cast<const void*>(bufferAddr), ioSize);
                break;
            default: Panic();
        }

        completedCount += ioSize;
        currentPos += ioSize;
    }

    return completedCount;
}

VFS::Vnode* Ext2::Create(VFS::Vnode* directory, const String& name, VFS::VnodeType vnodeType)
{
    auto directoryContext = static_cast<Inode*>(directory->context);
    Assert(directoryContext->typePermissions & Directory);

    uint32_t inodeNum = Allocate(AllocationType::Inode);
    Assert(inodeNum != 0);

    Inode* inode = GetInode(inodeNum);
    Assert(inode != nullptr);

    // TODO: Support file ACL and other fields in ext2 directory
    inode->hardLinksCount = 1;
    inode->size0 = 0;

    VFS::Vnode* vnode = VFS::ConstructVnode(inodeNum, this, inode, inode->size0, vnodeType);

    switch (vnode->type)
    {
        case VFS::VnodeType::RegularFile:
            WriteDirectoryEntry(directory, inodeNum, name, DEntryRegularFile);
            inode->typePermissions = DEFAULT_FILE_TYPE_PERMISSIONS;
            break;
        case VFS::VnodeType::Directory:
            inode->typePermissions = DEFAULT_DIRECTORY_TYPE_PERMISSIONS;
            WriteDirectoryEntry(directory, inodeNum, name, DEntryDirectory);
            WriteDirectoryEntry(vnode, vnode->inodeNum, String("."), DEntryDirectory);
            WriteDirectoryEntry(vnode, directory->inodeNum, String(".."), DEntryDirectory);
            break;
        default:
            Panic();
    }

    return vnode;
}

void Ext2::Truncate(VFS::Vnode* vnode)
{
    auto context = static_cast<Inode*>(vnode->context);
    Assert(context->size1 == 0);
    context->size0 = 0;
    vnode->fileSize = 0;
}

void Ext2::WriteDirectoryEntry(VFS::Vnode* directory, uint32_t inodeNum, const String& name, DirectoryEntryType type)
{
    uint8_t nameLength = name.GetLength();
    uint8_t nameLengthPadding = 4 - (nameLength % 4);

    Ext2::DirectoryEntry directoryEntry {};
    directoryEntry.typeIndicator = type;
    directoryEntry.inodeNum = inodeNum;
    directoryEntry.entrySize = sizeof(Ext2::DirectoryEntry) + nameLength + nameLengthPadding;
    directoryEntry.nameLength = nameLength;

    uint8_t zero = 0;
    Write(directory, &directoryEntry, sizeof(DirectoryEntry), directory->fileSize);
    Write(directory, name.ToRawString(), directoryEntry.nameLength, directory->fileSize);
    for (uint8_t i = 0; i < nameLengthPadding; ++i)
    {
        Write(directory, &zero, 1, directory->fileSize);
    }
}

uint32_t Ext2::GetBlockAddr(VFS::Vnode* vnode, uint32_t requestedBlockIndex, bool allocateMissingBlock)
{
    // TODO: Refactor, make recursive

    auto context = static_cast<Inode*>(vnode->context);

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
    }
    else if (requestedBlockIndex < (12 + pointersPerBlock * pointersPerBlock))
    {
        uint64_t index = requestedBlockIndex - (12 + pointersPerBlock);

        uint64_t doublyIndirectBlockReadPos = sizeof(blockPtr) * (index / pointersPerBlock);
        Read(context->doublyIndirectBlockPtr, &blockPtr, sizeof(blockPtr), doublyIndirectBlockReadPos);

        uint64_t singlyIndirectBlockReadPos = sizeof(blockPtr) * (index % pointersPerBlock);
        Read(blockPtr, &blockPtr, sizeof(blockPtr), singlyIndirectBlockReadPos);
    }
    else if (requestedBlockIndex < (12 + pointersPerBlock * pointersPerBlock * pointersPerBlock))
    {
        Panic();
    }
    else
    {
        Panic();
    }

    if (allocateMissingBlock && blockPtr == 0)
    {
        blockPtr = Allocate(AllocationType::Block);
        Assert(requestedBlockIndex < 12);
        context->directBlockPointers[requestedBlockIndex] = blockPtr;
    }

    return blockPtr;
}

uint32_t Ext2::Allocate(AllocationType allocationType)
{
    uint32_t object = 0;

    for (uint32_t blockGroupIndex = 0; blockGroupIndex < blockGroupsCount; ++blockGroupIndex)
    {
        BlockGroupDescriptor& blockGroup = blockGroupDescTable[blockGroupIndex];
        if (blockGroup.unallocatedBlocksCount > 0)
        {
            uint64_t usageBitmapBlock;
            uint64_t objectsPerBlockGroup;
            switch (allocationType)
            {
                case AllocationType::Inode:
                    usageBitmapBlock = blockGroup.inodeUsageBitmapBlock;
                    objectsPerBlockGroup = superblock->inodesPerBlockGroup;
                    break;
                case AllocationType::Block:
                    usageBitmapBlock = blockGroup.blockUsageBitmapBlock;
                    objectsPerBlockGroup = superblock->blocksPerBlockGroup;
                    break;
                default: Panic();
            }

            uint64_t usageBitmapSize = superblock->blocksPerBlockGroup / 8;
            Assert(superblock->blocksPerBlockGroup % 8 == 0);

            auto usageBitmapBuffer = new uint8_t[usageBitmapSize];
            uint64_t usageBitmapDiskAddr = usageBitmapBlock * blockSize;
            disk->Read(usageBitmapDiskAddr, usageBitmapBuffer, usageBitmapSize);

            Bitmap usageBitmap(usageBitmapBuffer, usageBitmapSize, false);

            bool found = false;
            for (uint32_t i = 0; i < usageBitmapSize * 8; ++i)
            {
                if (!usageBitmap.GetBit(i))
                {
                    object = blockGroupIndex * objectsPerBlockGroup + i;

                    usageBitmap.SetBit(i, true);
                    disk->Write(usageBitmapDiskAddr, usageBitmapBuffer, usageBitmapSize);

                    switch (allocationType)
                    {
                        case AllocationType::Inode:
                            blockGroup.unallocatedInodesCount--;
                            superblock->unallocatedInodesCount--;
                            object++; // Inode numbers start from 1
                            break;
                        case AllocationType::Block:
                            blockGroup.unallocatedBlocksCount--;
                            superblock->unallocatedBlocksCount--;
                            break;
                        default: Panic();
                    }

                    found = true;
                    break;
                }
            }

            delete[] usageBitmapBuffer;
            if (found) break;
        }
    }

    return object;
}

Ext2::Inode* Ext2::GetInode(uint32_t inodeNum)
{
    // Inode numbers start at 1
    uint32_t blockGroupIndex = (inodeNum - 1) / superblock->inodesPerBlockGroup;
    uint32_t inodeIndex = (inodeNum - 1) % superblock->inodesPerBlockGroup;

    uint64_t inodeTableDiskAddr = blockGroupDescTable[blockGroupIndex].inodeTableStartBlock * blockSize;
    uint64_t diskAddr = inodeTableDiskAddr + (inodeIndex * superblock->inodeSize);

    auto inode = new (Allocator::Permanent) Inode;
    disk->Read(diskAddr, inode, sizeof(Inode));

    return inode;
}

String Ext2::GetPathFromSymbolicLink(VFS::Vnode* symLinkVnode)
{
    Assert(symLinkVnode->type == VFS::VnodeType::SymbolicLink);

    char* string = new char[symLinkVnode->fileSize + 1];
    string[symLinkVnode->fileSize] = '\0';

    // Symbolic link paths are stored in the 60 bytes taken in the inode by the 12 direct and 3 indirect
    // block pointers, but only if the length of the path (file size in the inode) is under 60.
    Assert(symLinkVnode->fileSize <= 60);

    auto context = static_cast<Inode*>(symLinkVnode->context);
    uint64_t remainingChars = symLinkVnode->fileSize;
    uint64_t readCount;

    Assert(sizeof(context->directBlockPointers) == 48);
    remainingChars -= (readCount = Min(remainingChars, sizeof(context->directBlockPointers)));
    memcpy(string, context->directBlockPointers, readCount);

    Assert(sizeof(context->singlyIndirectBlockPtr) == 4);
    remainingChars -= (readCount = Min(remainingChars, sizeof(context->singlyIndirectBlockPtr)));
    memcpy(string + 48, &context->singlyIndirectBlockPtr, readCount);

    Assert(sizeof(context->doublyIndirectBlockPtr) == 4);
    remainingChars -= (readCount = Min(remainingChars, sizeof(context->doublyIndirectBlockPtr)));
    memcpy(string + 52, &context->doublyIndirectBlockPtr, readCount);

    Assert(sizeof(context->triplyIndirectBlockPtr) == 4);
    remainingChars -= readCount = Min(remainingChars, sizeof(context->triplyIndirectBlockPtr));
    memcpy(string + 56, &context->triplyIndirectBlockPtr, readCount);

    Assert(remainingChars == 0);

    return String(string);
}

Ext2::~Ext2()
{
    delete superblock;
    delete[] blockGroupDescTable;
    Panic();
}
