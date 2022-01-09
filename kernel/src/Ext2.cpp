#include "Ext2.h"
#include <stdint.h>
#include "Serial.h"

struct Ext2Superblock
{
    uint32_t inodesCount;
    uint32_t blocksCount;
    uint32_t superuserReservedBlocksCount;
    uint32_t unallocatedBlocksCount;
    uint32_t unallocatedInodesCount;
    uint32_t superblockBlock;
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
    uint16_t minorVersion;
    uint32_t lastConsistencyCheckTime; // in POSIX time
    uint32_t forcedConsistencyChecksInterval;
    uint32_t originOperatingSystemId;
    uint32_t majorVersion;
    uint16_t reservedBlocksUserId;
    uint16_t reservedBlocksGroupId;
};

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
    uint32_t directBlockPtr0;
    uint32_t directBlockPtr1;
    uint32_t directBlockPtr2;
    uint32_t directBlockPtr3;
    uint32_t directBlockPtr4;
    uint32_t directBlockPtr5;
    uint32_t directBlockPtr6;
    uint32_t directBlockPtr7;
    uint32_t directBlockPtr8;
    uint32_t directBlockPtr9;
    uint32_t directBlockPtr10;
    uint32_t directBlockPtr11;
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
    void SetSize(uint64_t size);
    uint64_t GetSize();
} __attribute__((packed));

struct Ext2DirectoryEntry
{
    uint32_t inodeNum;
    uint16_t entrySize;
    uint8_t nameLength0;
    uint8_t nameLength1; // Type indicator if feature "directory entries have file type byte" is set
    char* name;
} __attribute__((packed));

const uint32_t INODE_ROOT_DIR = 2;

uint32_t blockGroupsCount;
uint32_t inodesCount;
uint32_t blocksCount;
uint64_t blockSize;
uint32_t blocksPerBlockGroup;
uint32_t inodesPerBlockGroup;
uint32_t inodeSize;
uint16_t minorVersion;
uint32_t majorVersion;
Ext2BlockGroupDescriptor* blockGroupDescTable;
Ext2Inode* rootDirInode;

// TODO: Get rid of this when loading Ext2 from disk is implemented
uint64_t ramDiskAddr;

Ext2Inode* GetInode(uint32_t inodeNum)
{
    // Inode indexes start at 1
    uint32_t blockGroupIndex = (inodeNum - 1) / inodesPerBlockGroup;
    uint32_t inodeIndex = (inodeNum - 1) % inodesPerBlockGroup;

    Serial::Printf("Inode table Start: %x", blockGroupDescTable[blockGroupIndex].inodeTableStartBlock);
    uint64_t diskAddr = blockGroupDescTable[blockGroupIndex].inodeTableStartBlock + inodeIndex * inodeSize;

    Serial::Printf("Disk adddr : %x", diskAddr);

    //diskAddr = 0x400;
    // TODO: If loading Ext2 from disk, read from disk and allocate space for it on the heap
    Ext2Inode* inode = (Ext2Inode*)(diskAddr + ramDiskAddr);
    Serial::Printf("-------------Inode: %x", inode->typePermissions);
    return (Ext2Inode*)(diskAddr + ramDiskAddr);
}

void InitializeExt2(uint64_t ramDiskBegin, uint64_t ramDiskEnd)
{
    Serial::Print("Initializing Ext2 file system...");

    // TODO: Eventually switch from RAM disk to real disk
    Serial::Printf("Ext2 RAM disk size: %x", ramDiskEnd - ramDiskBegin);
    ramDiskAddr = ramDiskBegin;

    // TODO: If loading Ext2 from disk, read from disk and allocate 1024B for it on the heap
    uint64_t superblock = ramDiskAddr + 1024;

    uint16_t ext2Signature = *(uint16_t*)(superblock + 56);
    if (ext2Signature != 0xef53)
    {
        Serial::Print("Invalid ext2 signature!");
        Serial::Print("Hanging...");
        while (true) asm("hlt");
    }

    uint16_t fileSystemState = *(uint16_t*)(superblock + 58);
    if (fileSystemState != 1)
    {
        Serial::Print("File system state is not clean.");
        Serial::Print("Hanging...");
        while (true) asm("hlt");
    }

    // Parse from superblock
    inodesCount = *(uint32_t*)superblock;
    blocksCount = *(uint32_t*)(superblock + 4);
    blockSize = 1024 << *(uint32_t*)(superblock + 24);
    blocksPerBlockGroup = *(uint32_t*)(superblock + 32);
    inodesPerBlockGroup = *(uint32_t*)(superblock + 40);

    Serial::Printf("Inodes count: %d", inodesCount);
    Serial::Printf("Blocks count: %d", blocksCount);
    Serial::Printf("Block size: %d", blockSize);
    Serial::Printf("Blocks per block group: %d", blocksPerBlockGroup);
    Serial::Printf("Inodes per block group: %d", inodesPerBlockGroup);

    inodeSize = 128;
    minorVersion = *(uint16_t*)(superblock + 62);
    majorVersion = *(uint32_t*)(superblock + 76);

    Serial::Printf("Version: %d.", majorVersion, "");
    Serial::Printf("%d", minorVersion);

    if (majorVersion >= 1)
    {
        // TODO: Better support for extended
        inodeSize = *(uint32_t*)(superblock + 88);
    }

    // TODO: If loading Ext2 from disk, free superblock allocate on the heap

    blockGroupsCount = blocksCount / blocksPerBlockGroup + (blocksCount % blocksPerBlockGroup > 0 ? 1 : 0);
    uint32_t blockGroupsCountCheck = inodesCount / inodesPerBlockGroup + (inodesCount % inodesPerBlockGroup > 0 ? 1 : 0);

    if (blockGroupsCount != blockGroupsCountCheck)
    {
        Serial::Print("Block group count could not be calculated.");
        Serial::Print("Hanging...");
        while (true) asm("hlt");
    }

    Serial::Printf("Block group count: %d", blockGroupsCount);

    // TODO: If loading Ext2 from disk, allocate space for it on the heap
    uint32_t blockGroupDescTableDiskAddr = blockSize * (blockSize == 1024 ? 2 : 1);
    blockGroupDescTable = (Ext2BlockGroupDescriptor*)(ramDiskBegin + blockGroupDescTableDiskAddr);

    Serial::Printf("BGDT disk addr: %x", blockGroupDescTableDiskAddr);
    Serial::Printf("element size: %d", sizeof(Ext2BlockGroupDescriptor));

    rootDirInode = GetInode(INODE_ROOT_DIR);
    Serial::Printf("Root dir inode: %x", *(uint64_t*)rootDirInode);
    Vector<TNode> rootDirContents = GetDirectoryListing();

    Serial::Printf("Root directory file/subdirectory count: %d", rootDirContents.GetLength());
    for (uint64_t i = 0; i < rootDirContents.GetLength(); ++i)
    {
        Serial::Print(rootDirContents[i].name);
    }
}

// TODO: Support directories other than root
Vector<TNode> GetDirectoryListing()
{
    Vector<TNode> tNodes;
    // TODO: Support more than 1 block
    uint64_t currentBlock = rootDirInode->directBlockPtr0;
    Serial::Printf("current block: %d", currentBlock);
    uint64_t diskAddr = currentBlock * blockSize;

    //Serial::Printf("Disk addr: %x", diskAddr);

    Ext2DirectoryEntry* directoryEntry = 0;
    while (true)
    {
        // TODO: If loading Ext2 from disk, read from disk
        directoryEntry = (Ext2DirectoryEntry*)(diskAddr + ramDiskAddr);

        Serial::Printf("Directory entry: %d", directoryEntry->entrySize);
        if (directoryEntry->entrySize == 0) break;

        // TODO: Inode here
        tNodes.Push(TNode(directoryEntry->name, 0));

        diskAddr += directoryEntry->entrySize;
    }

    return tNodes;
}