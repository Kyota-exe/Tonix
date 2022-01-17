#ifndef MISKOS_VFS_H
#define MISKOS_VFS_H

#include "Vector.h"

namespace VFS
{
    int Open(const char* path);

    struct VNode
    {
        char* name;
        uint32_t inodeNum;
        Vector<VNode> children;

        VNode(const char* _name, uint32_t _inodeNum);
        VNode(const VNode &original);
        VNode& operator=(const VNode& newValue);
        ~VNode();
    };
}

#endif
