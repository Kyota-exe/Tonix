#include "VFS.h"
#include "Heap.h"
#include "Memory.h"
#include "StringUtilities.h"

TNode::TNode(const char* _name, Inode* _inode)
{
    uint64_t nameLength = StringLength(_name);
    name = (char*)KMalloc(nameLength + 1);
    MemCopy(name, (void*)_name, nameLength);
    name[nameLength] = 0;

    inode = _inode;
}

TNode::TNode(const TNode &original)
{
    uint64_t nameLength = StringLength(original.name);
    name = (char*)KMalloc(nameLength + 1);
    MemCopy(name, original.name, nameLength);
    name[nameLength] = 0;

    inode = original.inode;
}

TNode::~TNode()
{
    KFree(name);
}

TNode& TNode::operator=(const TNode newValue)
{
    KFree(name);
    uint64_t nameLength = StringLength(newValue.name);
    name = (char*)KMalloc(nameLength + 1);
    MemCopy(name, newValue.name, nameLength);
    name[nameLength] = 0;

    inode = newValue.inode;
    return *this;
}