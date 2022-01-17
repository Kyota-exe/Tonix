#ifndef MISKOS_VFS_H
#define MISKOS_VFS_H

#include "Vector.h"

namespace VFS
{
    int Open(char* path);
    uint64_t Read(int descriptor, void* buffer, uint64_t count);

    struct VNode
    {
        char* name;
        uint32_t inodeNum;

        VNode(const char* _name = "", uint32_t _inodeNum = 0);
        VNode(const VNode &original);
        VNode& operator=(const VNode& newValue);
        ~VNode();
    };
}

#endif
