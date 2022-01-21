#include "Ext2Inode.h"
#include "Memory/Memory.h"
#include "Serial.h"
#include "Ext2.h"
#include "Bitmap.h"
#include "Panic.h"

namespace Ext2
{
    enum InodeTypePermissions
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

    enum DirectoryEntryType
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

    const uint16_t DEFAULT_FILE_TYPE_PERMISSIONS = RegularFile | UserRead | UserWrite | GroupRead | OtherRead;

    uint64_t Ext2Inode::Read(void* buffer, uint64_t count, uint64_t readPos)
    {
        // TODO: Support files larger than 2^32 bytes
        if (readPos + count > size0)
        {
            count = size0 - readPos;
        }

        uint64_t currentReadPos = readPos;
        uint64_t parsedCount = 0;
        while (parsedCount < count)
        {
            uint64_t remainingBytes = count - parsedCount;
            uint64_t offsetInBlock = currentReadPos % blockSize;
            uint64_t memCopySize = blockSize - offsetInBlock;
            if (memCopySize > remainingBytes) memCopySize = remainingBytes;

            uint64_t diskAddr = GetBlockAddr(currentReadPos / blockSize, false) * blockSize + offsetInBlock;
            MemCopy((void*)((uint64_t)buffer + parsedCount), (void*)(diskAddr + ramDiskAddr), memCopySize);

            parsedCount += memCopySize;
            readPos += memCopySize;
        }

        return parsedCount;
    }

    uint64_t Ext2Inode::Write(void* buffer, uint64_t count, uint64_t writePos)
    {
        // TODO: DRY
        uint64_t currentWritePos = writePos;
        uint64_t wroteCount = 0;
        while (wroteCount < count)
        {
            uint64_t remainingBytes = count - wroteCount;
            uint64_t offsetInBlock = currentWritePos % blockSize;
            uint64_t memCopySize = blockSize - offsetInBlock;
            if (memCopySize > remainingBytes) memCopySize = remainingBytes;

            uint64_t diskAddr = GetBlockAddr(currentWritePos / blockSize, true) * blockSize + offsetInBlock;
            MemCopy((void*)(diskAddr + ramDiskAddr), (void*)((uint64_t)buffer + wroteCount), memCopySize);

            wroteCount += memCopySize;
            currentWritePos += memCopySize;
            if (size0 < currentWritePos) size0 = currentWritePos;
        }

        return wroteCount;
    }

    uint64_t Ext2Inode::Write(uint64_t value, uint64_t count, uint64_t writePos, bool updateFileSize)
    {
        // TODO: DRY
        uint64_t currentWritePos = writePos;
        uint64_t wroteCount = 0;
        while (wroteCount < count)
        {
            uint64_t remainingBytes = count - wroteCount;
            uint64_t offsetInBlock = currentWritePos % blockSize;
            uint64_t memsetSize = blockSize - offsetInBlock;
            if (memsetSize > remainingBytes) memsetSize = remainingBytes;

            uint64_t diskAddr = GetBlockAddr(currentWritePos / blockSize, true) * blockSize + offsetInBlock;
            Memset((void*)(diskAddr + ramDiskAddr), value, memsetSize);

            wroteCount += memsetSize;
            currentWritePos += memsetSize;
            if (updateFileSize && size0 < currentWritePos) size0 = currentWritePos;
        }

        return wroteCount;
    }

    uint32_t Ext2Inode::Create(char* name)
    {
        KAssert(typePermissions & 0x4000, "Inode must be a directory to create files in it.");

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
                Bitmap inodeUsageBitmap
                {
                    (uint8_t*)(blockGroup->inodeUsageBitmapBlock * blockSize + ramDiskAddr),
                    inodeUsageBitmapSize
                };

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

        // TODO: Support permissions, file ACL and other fields in ext2 inode
        inode->typePermissions = DEFAULT_FILE_TYPE_PERMISSIONS;
        inode->hardLinksCount = 1;

        uint8_t nameLength = String::Length(name);
        Ext2DirectoryEntry directoryEntry
        {
            inodeNum,sizeof(Ext2DirectoryEntry),
            nameLength,DirectoryEntryType::DEntryRegularFile
        };
        Write(&directoryEntry, sizeof(DirectoryEntryType), size0);
        Write(name, nameLength, size0);
        Write(0, 4 - (nameLength % 4), size0, true);

        return inodeNum;
    }

    Vector<VNode> Ext2Inode::GetDirectoryListing()
    {
        Vector<VNode> vNodes;

        uint64_t parsedLength = 0;
        while (parsedLength < size0)
        {
            Ext2DirectoryEntry directoryEntry;
            Read(&directoryEntry, sizeof(Ext2DirectoryEntry), parsedLength);

            char name[directoryEntry.nameLength + 1];
            Read(name, directoryEntry.nameLength, parsedLength + sizeof(directoryEntry));
            name[directoryEntry.nameLength] = 0;

            if (directoryEntry.inodeNum != 0)
            {
                vNodes.Push(VNode(name, directoryEntry.inodeNum));
            }

            parsedLength += directoryEntry.entrySize;
        }
        return vNodes;
    }

    uint32_t Ext2Inode::GetBlockAddr(uint32_t requestedBlockIndex, bool allocateMissingBlock)
    {
        uint64_t pointersPerBlock = blockSize / sizeof(uint32_t);
        uint32_t block = 0;

        if (requestedBlockIndex < 12)
        {
            block = directBlockPointers[requestedBlockIndex];
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
                    Bitmap blockUsageBitmap
                    {
                            (uint8_t*)(blockGroup->blockUsageBitmapBlock * blockSize + ramDiskAddr),
                            blockUsageBitmapSize
                    };

                    for (uint32_t blockIndex = 0; blockIndex < blockUsageBitmap.size * 8; ++blockIndex)
                    {
                        if (!blockUsageBitmap.GetBit(blockIndex))
                        {
                            block = blockGroupIndex * superblock->blocksPerBlockGroup + blockIndex;
                            blockUsageBitmap.SetBit(blockIndex, true);
                            blockGroup->unallocatedBlocksCount--;
                            superblock->unallocatedBlocksCount--;
                            directBlockPointers[requestedBlockIndex] = block;
                            return block;
                        }
                    }
                }
            }
        }

        return block;
    }
}