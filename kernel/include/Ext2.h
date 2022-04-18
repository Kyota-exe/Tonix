#pragma once

#include "VFS.h"
#include "Vector.h"
#include "FileSystem.h"
#include "Ext2Structs.h"

class Ext2 : public FileSystem
{
public:
    uint64_t Read(Vnode* vnode, void* buffer, uint64_t count, uint64_t readPos) override;
    uint64_t Write(Vnode* vnode, const void* buffer, uint64_t count, uint64_t writePos) override;
    Vnode* FindInDirectory(Vnode* directory, const String& name) override;
    void Create(Vnode* vnode, Vnode* directory, const String& name) override;
    void Truncate(Vnode* vnode) override;
    explicit Ext2(Disk* disk);
    ~Ext2() override;

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

    enum class Ext2AllocationType
    {
        Inode, Block
    };

    enum class Ext2IOType
    {
        Read, Write
    };

    uint32_t GetBlockAddr(Vnode* vnode, uint32_t requestedBlockIndex, bool allocateMissingBlock);
    Ext2Inode* GetInode(uint32_t inodeNum);
    void WriteDirectoryEntry(Vnode* directory, uint32_t inodeNum, const String& name, Ext2DirectoryEntryType type);
    Vnode* CacheDirectoryEntry(const Ext2DirectoryEntry& directoryEntry);
    uint64_t Read(uint32_t block, void* buffer, uint64_t count, uint64_t readPos);
    uint32_t Allocate(Ext2AllocationType allocationType);
    uint64_t DiskOperation(Ext2IOType ioType, Vnode* vnode, void* buffer, uint64_t count, uint64_t position);

    uint32_t blockGroupsCount;
    uint64_t blockSize;
    Ext2Superblock* superblock;
    Ext2BlockGroupDescriptor* blockGroupDescTable;
};