#ifndef MISKOS_EXT2_H
#define MISKOS_EXT2_H

#include "VFS.h"

void InitializeExt2(uint64_t ramDiskBegin, uint64_t ramDiskEnd);
Vector<TNode> GetDirectoryListing();

#endif
