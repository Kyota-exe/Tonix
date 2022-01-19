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

    const uint16_t DEFAULT_FILE_TYPE_PERMISSIONS = RegularFile | UserRead | UserWrite | GroupRead | OtherRead;

    uint64_t Ext2Inode::Read(void* buffer, uint64_t count, uint64_t readPos)
    {
        // TODO: Support sizes bigger than 2^32 bytes
        if (readPos + count > size0)
        {
            count = size0 - readPos;
        }

        uint64_t currentReadPos = readPos;
        uint64_t parsedCount = 0;
        while (parsedCount < count)
        {
            uint64_t remainingBytes = (count - parsedCount);
            uint64_t offsetInBlock = (currentReadPos % blockSize);
            uint64_t memCopySize = blockSize - offsetInBlock;
            if (memCopySize > remainingBytes) memCopySize = remainingBytes;

            uint64_t diskAddr = GetBlockAddr(currentReadPos / blockSize) * blockSize + offsetInBlock;
            MemCopy(buffer, (void*)(diskAddr + ramDiskAddr), memCopySize);

            parsedCount += memCopySize;
        }

        return parsedCount;
    }

    uint64_t Ext2Inode::Write(void* buffer, uint64_t count, uint64_t writePos)
    {
        // Create new bool arg in GetBlockAddr that allocates new block to ptr if it is not already present.
        uint64_t currentWritePos = writePos;
        uint64_t wroteCount = 0;
        while (wroteCount < count)
        {
            uint64_t remainingBytes = (count - wroteCount);
            uint64_t offsetInBlock = (currentWritePos % blockSize);
            uint64_t memCopySize = blockSize - offsetInBlock;
            if (memCopySize > remainingBytes) memCopySize = remainingBytes;

            uint64_t diskAddr = GetBlockAddr(currentWritePos / blockSize) * blockSize + offsetInBlock;
            MemCopy((void*)(diskAddr + ramDiskAddr), buffer, memCopySize);

            wroteCount += memCopySize;
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
                        inodeNum = blockGroupIndex * superblock->inodesPerBlockGroup + inodeIndex;
                        inode = GetInode(inodeNum);
                        inodeUsageBitmap.SetBit(inodeIndex, true);
                        blockGroup->unallocatedInodesCount--;
                        goto FoundUnallocatedInode;
                    }
                }
            }
        }

        FoundUnallocatedInode:

        KAssert(inode != nullptr && inodeNum != 0, "Could not create new file, unallocated inode not found.");

        Serial::Print("\n\nDetails:");
        Serial::Printf("Number: %d", inodeNum);
        Serial::Printf("Size: %d", inode->size0);
        Serial::Printf("User ID: %d", inode->userId);
        Serial::Printf("Generation Number: %d", inode->generationNumber);
        Serial::Printf("Hard links count: %d", inode->hardLinksCount);
        Serial::Printf("Type/Permissions: %x", inode->typePermissions);
        Serial::Printf("Creation time: %d", inode->creationTime);
        Serial::Printf("DirectPointers[0]: %x", inode->directBlockPointers[0]);
        Serial::Printf("Fragment number: %d", inode->fragmentNumber);

        // TODO: Support permissions, file ACL and other fields in ext2 inode
        inode->typePermissions = DEFAULT_FILE_TYPE_PERMISSIONS;
        inode->hardLinksCount = 1;


        /*
        // Find unallocated blocks
        uint64_t blockUsageBitmapSize = blockGroup->unallocatedBlocksCount / 8;
        if (blockGroup->unallocatedBlocksCount % 8 != 0) blockUsageBitmapSize++;
        Bitmap blockUsageBitmap
        {
            (uint8_t*)(blockGroup->blockUsageBitmapBlock + ramDiskAddr),
            blockGroup->unallocatedBlocksCount
        };
         */

        while (true) asm("cli\nhlt\n");
        return 3;

        return inodeNum;
    }

    uint32_t Ext2Inode::GetBlockAddr(uint32_t currentBlockIndex)
    {
        uint64_t pointersPerBlock = blockSize / sizeof(uint32_t);

        if (currentBlockIndex < 12)
        {
            return directBlockPointers[currentBlockIndex];
        }

        if (currentBlockIndex < 12 + pointersPerBlock)
        {
            Serial::Print("Singly indirect pointers are not yet supported.");
            Serial::Print("Hanging...");
            while (true) asm("hlt");
        }

        if (currentBlockIndex < 12 + pointersPerBlock * pointersPerBlock)
        {
            Serial::Print("Doubly indirect pointers are not yet supported.");
            Serial::Print("Hanging...");
            while (true) asm("hlt");
        }

        if (currentBlockIndex < 12 + pointersPerBlock * pointersPerBlock * pointersPerBlock)
        {
            Serial::Print("Triply indirect pointers are not yet supported.");
            Serial::Print("Hanging...");
            while (true) asm("hlt");
        }

        Serial::Printf("Unsupported file contents block index (%d).", currentBlockIndex);
        Serial::Print("Hanging...");
        while (true) asm("hlt");
    }
}