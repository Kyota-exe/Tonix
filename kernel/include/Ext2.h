#ifndef MISKOS_EXT2_H
#define MISKOS_EXT2_H

#include "VFS.h"
#include "Ext2Inode.h"

Vector<VNode> GetDirectoryListing(Ext2Inode* directoryInode);
void InitializeExt2(uint64_t ramDiskBegin, uint64_t ramDiskEnd);

#endif
