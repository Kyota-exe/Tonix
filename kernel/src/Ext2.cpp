#include "Ext2.h"
#include <stdint.h>
#include "Memory/Memory.h"
#include "Serial.h"
#include "StringUtilities.h"
#include "Panic.h"

namespace Ext2
{
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
    }
}