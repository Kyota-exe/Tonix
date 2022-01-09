#ifndef MISKOS_VFS_H
#define MISKOS_VFS_H

#include "Vector.h"

struct Inode
{
    uint64_t inodeNumber;
};

struct TNode
{
    char* name;
    Inode* inode;
    TNode(const char* _name, Inode* _inode);
    TNode(const TNode &original);
    TNode& operator=(const TNode newValue);
    ~TNode();
};

struct DirectoryInode : Inode
{
    Vector<TNode> fileTNodes;
    Vector<TNode> subdirectoryTNodes;
};

struct FileInode : Inode
{
    uint64_t fileSize;
};

#endif
