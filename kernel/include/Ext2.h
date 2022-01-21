#ifndef MISKOS_EXT2_H
#define MISKOS_EXT2_H

#include "VFS.h"
#include "Ext2Inode.h"
#include "Vector.h"

namespace Ext2
{
    struct Ext2Superblock
    {
        uint32_t inodesCount;
        uint32_t blocksCount;
        uint32_t superuserReservedBlocksCount;
        uint32_t unallocatedBlocksCount;
        uint32_t unallocatedInodesCount;
        uint32_t superblockBlock; // Block containing the superblock
        uint32_t blockSizeLog2Minus10; // 1024 << n = block size
        uint32_t fragmentSizeLog2Minus10; // 1024 << n = fragment size
        uint32_t blocksPerBlockGroup;
        uint32_t fragmentsPerBlockGroup;
        uint32_t inodesPerBlockGroup;
        uint32_t lastMountTime; // in POSIX time
        uint32_t lastWrittenTime; // in POSIX time
        uint16_t volumeMountedCount; // Number of times the volume has been mounted since its last consistency check
        uint16_t mountsAllowedBeforeConsistencyCheck; // Number of mounts allowed before a consistency check must be done
        uint16_t ext2Signature;
        uint16_t fileSystemState;
        uint16_t errorHandlingMethod;
        uint16_t minorVersion; // Version fractional part
        uint32_t lastConsistencyCheckTime; // in POSIX time
        uint32_t forcedConsistencyChecksInterval; // in POSIX time
        uint32_t originOperatingSystemId; // Operating system ID from which the filesystem on this volume was created
        uint32_t majorVersion; // Version integer part
        uint16_t reservedBlocksUserId;
        uint16_t reservedBlocksGroupId;

        // Extended superblock fields
        uint32_t firstNonReservedInode; // Inode numbers start at 1
        uint16_t inodeSize;
        uint16_t superblockBlockGroup; // Block group that this superblock is part of if this is a backup superblock
        uint32_t optionalFeatures;
        uint32_t requiredFeatures;
        uint32_t readOnlyFeatures; // Features that if not supported, the volume must be mounted read-only
        uint8_t fileSystemId[16];
        char volumeName[16]; // Null-terminated volume name
        char lastMountedPath[64]; // Null-terminated name of path the volume was last mounted to
        uint32_t compressionAlgorithm;
        uint8_t preallocFilesBlocksCount; // Number of blocks to preallocate for files
        uint8_t preallocDirectoriesBlocksCount; // Number of blocks to preallocate for directories
        uint16_t unused;
        uint8_t journalId[16];
        uint32_t journalInode;
        uint32_t journalDevice;
        uint32_t orphanInodeListHead;
    } __attribute__((packed));

    struct Ext2BlockGroupDescriptor
    {
        uint32_t blockUsageBitmapBlock;
        uint32_t inodeUsageBitmapBlock;
        uint32_t inodeTableStartBlock;
        uint16_t unallocatedBlocksCount;
        uint16_t unallocatedInodesCount;
        uint16_t directoriesCount;
        uint8_t unused[14];
    } __attribute__((packed));

    struct Ext2DirectoryEntry
    {
        uint32_t inodeNum;
        uint16_t entrySize;
        uint8_t nameLength; // Name length least-significant 8 bits
        uint8_t typeIndicator; // Name length most-significant 8 bits if feature "directory entries have file type byte" is not set
    } __attribute__((packed));

    extern Ext2Inode* rootDirInode;
    extern uint32_t blockGroupsCount;
    extern uint64_t blockSize;
    extern uint64_t ramDiskAddr;
    extern Ext2Superblock* superblock;
    extern Ext2BlockGroupDescriptor* blockGroupDescTable;

    Ext2Inode* GetInode(uint32_t inodeNum);
    Vector<VNode> GetDirectoryListing(Ext2Inode* directoryInode);
    void Initialize(uint64_t ramDiskBegin, uint64_t ramDiskEnd);
}

#endif
