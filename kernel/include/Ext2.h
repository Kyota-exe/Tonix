#ifndef MISKOS_EXT2_H
#define MISKOS_EXT2_H

#include "VFS.h"
#include "Ext2Inode.h"

using namespace VFS;

namespace Ext2
{
    extern Ext2Inode* rootDirInode;
    extern uint64_t blockSize;
    extern uint64_t ramDiskAddr;

    Ext2Inode* GetInode(uint32_t inodeNum);
    Vector<VNode> GetDirectoryListing(Ext2Inode* directoryInode);
    void Initialize(uint64_t ramDiskBegin, uint64_t ramDiskEnd);
}

#endif
