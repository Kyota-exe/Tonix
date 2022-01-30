#pragma once

#include "VFS.h"
#include "Vector.h"

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

struct Ext2Inode
{
    uint16_t typePermissions;
    uint16_t userId;
    uint32_t size0;
    uint32_t lastAccessTime;
    uint32_t creationTime;
    uint32_t lastModificationTime;
    uint32_t deletionTime;
    uint16_t groupId;
    uint16_t hardLinksCount;
    uint32_t diskSectorsCount;
    uint32_t flags;
    uint32_t reserved0; // Reserved by Linux, customizable
    uint32_t directBlockPointers[12];
    uint32_t singlyIndirectBlockPtr; // Points to a block that is a list of direct block pointers
    uint32_t doublyIndirectBlockPtr; // Points to a block that is a list of pointers to singly indirect block pointers
    uint32_t triplyIndirectBlockPtr; // Points to a block that is a list of pointers to doubly indirect block pointers
    uint32_t generationNumber;
    uint32_t fileACLBlockPtr;
    uint32_t size1; // If the file is a directory, this field is a pointer to a directory ACL
    uint32_t fragmentBlockAddr;
    uint8_t fragmentNumber; // Customizable
    uint8_t fragmentSize; // Customizable
    uint16_t reserved1; // Customizable
    uint16_t userId1; // Customizable
    uint16_t groupId1; // Customizable
    uint32_t reserved2; // Customizable
} __attribute__((packed));

class Ext2 : public FileSystem
{
public:
    void Mount(Vnode* mountPoint) override;
    uint64_t Read(Vnode* vnode, void* buffer, uint64_t count, uint64_t readPos) override;
    uint64_t Write(Vnode* vnode, const void* buffer, uint64_t count, uint64_t writePos) override;
    Vnode* FindInDirectory(Vnode* directory, const String& name) override;
    void Create(Vnode* vnode, Vnode* directory, const String& name) override;
    void Truncate(Vnode* vnode) override;
    explicit Ext2(void* _ramDiskAddr);

private:
    enum Ext2DirectoryEntryType : uint8_t
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

    uint32_t GetBlockAddr(Vnode* vnode, uint32_t requestedBlockIndex, bool allocateMissingBlock);
    Ext2Inode* GetInode(uint32_t inodeNum);
    void WriteDirectoryEntry(Vnode* directory, uint32_t inodeNum, const String& name, Ext2DirectoryEntryType type);

    uint64_t ramDiskVirtAddr;

    uint32_t blockGroupsCount;
    uint64_t blockSize;
    Ext2Superblock* superblock;
    Ext2BlockGroupDescriptor* blockGroupDescTable;
};