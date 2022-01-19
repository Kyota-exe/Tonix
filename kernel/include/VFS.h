#ifndef MISKOS_VFS_H
#define MISKOS_VFS_H

struct VNode;

#include <stdint.h>
#include "Task.h"

enum VFSFlag
{
    OCreate = 0x200
};

int Open(char* path, int flags, Process* process);
uint64_t Read(int descriptor, void* buffer, uint64_t count, Process* process);
uint64_t Write(int descriptor, void* buffer, uint64_t count, Process* process);

struct VNode
{
    char* name;
    uint32_t inodeNum;

    explicit VNode(const char* _name = "", uint32_t _inodeNum = 0);
    VNode(const VNode &original);
    VNode& operator=(const VNode& newValue);
    ~VNode();
};

#endif
