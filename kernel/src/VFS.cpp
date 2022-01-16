#include "Memory/Memory.h"
#include "VFS.h"
#include "StringUtilities.h"

int Open()
{
    return 0;
}

VNode::VNode(const char* _name, uint32_t _inodeNum) : inodeNum(_inodeNum)
{
    uint64_t nameLength = StringLength(_name);
    name = (char*)KMalloc(nameLength + 1);
    MemCopy(name, (void*)_name, nameLength);
    name[nameLength] = 0;

    children = Vector<VNode>();
}

VNode::VNode(const VNode &original) : inodeNum(original.inodeNum), children(original.children)
{
    uint64_t nameLength = StringLength(original.name);
    name = (char*)KMalloc(nameLength + 1);
    MemCopy(name, original.name, nameLength);
    name[nameLength] = 0;
}

VNode::~VNode()
{
    KFree(name);
}

VNode& VNode::operator=(const VNode& newValue)
{
    if (&newValue != this)
    {
        KFree(name);
        uint64_t nameLength = StringLength(newValue.name);
        name = (char*)KMalloc(nameLength + 1);
        MemCopy(name, newValue.name, nameLength);
        name[nameLength] = 0;
    }

    return *this;
}