#include "Ext2.h"
#include <stdint.h>
#include "Memory/Memory.h"
#include "Serial.h"
#include "StringUtilities.h"
#include "Panic.h"

namespace Ext2
{
    struct Ext2DirectoryEntry
    {
        uint32_t inodeNum;
        uint16_t entrySize;
        uint8_t nameLength; // Name length least-significant 8 bits
        uint8_t typeIndicator; // Name length most-significant 8 bits if feature "directory entries have file type byte" is not set
    } __attribute__((packed));

    const uint32_t INODE_ROOT_DIR = 2;

    uint32_t blockGroupsCount;
    uint64_t blockSize;
    Ext2Superblock* superblock;
    Ext2BlockGroupDescriptor* blockGroupDescTable;
    Ext2Inode* rootDirInode;

    uint64_t ramDiskAddr;

    Ext2Inode* GetInode(uint32_t inodeNum)
    {
        // Inode indexes start at 1
        uint32_t blockGroupIndex = (inodeNum - 1) / superblock->inodesPerBlockGroup;
        uint32_t inodeIndex = (inodeNum - 1) % superblock->inodesPerBlockGroup;

        uint64_t diskAddr = blockGroupDescTable[blockGroupIndex].inodeTableStartBlock * blockSize + inodeIndex * superblock->inodeSize;

        return (Ext2Inode*)(diskAddr + ramDiskAddr);
    }

    VNode ConstructVNode(Ext2DirectoryEntry* directoryEntry)
    {
        // TODO: Add support for when "file type byte for directory entries" are not supported.
        char name[directoryEntry->nameLength + 1];
        MemCopy(name, (char*)((uint64_t)directoryEntry + sizeof(Ext2DirectoryEntry)), directoryEntry->nameLength);
        name[directoryEntry->nameLength] = 0;

        if (String::Length(name) != directoryEntry->nameLength)
        {
            Serial::Printf("Directory entry name length computed (%d) ", String::Length(name), "");
            Serial::Printf("is different from name length in directory entry (%d).", directoryEntry->nameLength);
        }

        return VNode(name, directoryEntry->inodeNum);
    }

    Vector<VNode> GetDirectoryListing(Ext2Inode* directoryInode)
    {
        Vector<VNode> vNodes;

        uint64_t parsedLength = 0;
        while (parsedLength < directoryInode->size0)
        {
            // TODO: Add support for when the byte at index 7 of the directory entry is the most-significant 8 bits of the name length
            uint8_t nameLengthLowByte;
            directoryInode->Read(&nameLengthLowByte, 1, parsedLength + 6);
            uint32_t trueEntrySize = 8 + nameLengthLowByte;

            auto directoryEntry = (Ext2DirectoryEntry*)KMalloc(trueEntrySize);
            directoryInode->Read(directoryEntry, trueEntrySize, parsedLength);

            if (directoryEntry->inodeNum != 0)
            {
                VNode newVnode = ConstructVNode(directoryEntry);
                vNodes.Push(newVnode);
            }

            parsedLength += directoryEntry->entrySize;

            delete directoryEntry;
        }

        return vNodes;
    }

    void Initialize(uint64_t ramDiskBegin, uint64_t ramDiskEnd)
    {
        Serial::Print("Initializing Ext2 file system...");

        Serial::Printf("Ext2 RAM disk size: %x", ramDiskEnd - ramDiskBegin);
        ramDiskAddr = ramDiskBegin;

        superblock = (Ext2Superblock*)(ramDiskAddr + 1024);

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

        Serial::Printf("First non-reserved inode in file system: %d", superblock->firstNonReservedInode);

        Serial::Printf("Optional features: %x", superblock->optionalFeatures);
        Serial::Printf("Required features: %x", superblock->requiredFeatures);
        Serial::Printf("Read-only features: %x", superblock->readOnlyFeatures);

        Serial::Printf("Number of blocks to preallocate for files: %d", superblock->preallocFilesBlocksCount);
        Serial::Printf("Number of blocks to preallocate for directories: %d", superblock->preallocDirectoriesBlocksCount);

        uint32_t blockGroupDescTableDiskAddr = blockSize * (blockSize == 1024 ? 2 : 1);
        blockGroupDescTable = (Ext2BlockGroupDescriptor*)(ramDiskBegin + blockGroupDescTableDiskAddr);

        rootDirInode = GetInode(INODE_ROOT_DIR);

        Vector<VNode> rootDirContents = GetDirectoryListing(rootDirInode);
        Serial::Print("\n/ contents:");
        uint32_t subDirInodeNum = 0;
        for (uint64_t i = 0; i < rootDirContents.GetLength(); ++i)
        {
            Serial::Printf("%d - ", rootDirContents[i].inodeNum, "");
            Serial::Print(rootDirContents[i].name);
            if (String::Equals(rootDirContents[i].name, "subdirectory-bravo"))
            {
                subDirInodeNum = rootDirContents[i].inodeNum;
            }
        }

        Vector<VNode> subDirContents = GetDirectoryListing(GetInode(subDirInodeNum));
        Serial::Print("\n/subdirectory-bravo/ contents:");
        uint64_t barInodeNum = 0;
        for (uint64_t i = 0; i < subDirContents.GetLength(); ++i)
        {
            Serial::Printf("%d - ", subDirContents[i].inodeNum, "");
            Serial::Print(subDirContents[i].name);
            if (String::Equals(subDirContents[i].name, "bar.txt"))
            {
                barInodeNum = subDirContents[i].inodeNum;
            }
        }

        Ext2Inode* barInode = GetInode(barInodeNum);
        char* textContents = new char[barInode->size0];
        barInode->Read(textContents, barInode->size0, 0);
        Serial::Print("\n/subdirectory-bravo/bar.txt contents:");
        Serial::Print(textContents);
    }
}