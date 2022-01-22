#ifndef MISKOS_VFS_H
#define MISKOS_VFS_H

#include <stdint.h>

struct VNode;
#include "Task.h"

enum VFSOpenFlag
{
    OCreate = 0x200,
    OAppend = 0x08,
    OTruncate = 0x1000
};

enum VFSSeekType
{
    SeekSet = 0,
    SeekCursor = 1,
    SeekEnd = 2
};

int Open(const char* path, int flags, Process* process);
uint64_t Read(int descriptor, void* buffer, uint64_t count, Process* process);
uint64_t Write(int descriptor, const void* buffer, uint64_t count, Process* process);
void Close(int descriptor, Process* process);
uint64_t RepositionOffset(int descriptor, uint64_t offset, VFSSeekType seekType, Process* process);

struct VNode
{
    char* name;
    uint32_t inodeNum;

    explicit VNode(const char* _name = "", uint32_t _inodeNum = 0);
    VNode(const VNode& original);
    VNode& operator=(const VNode& newValue);
    ~VNode();
};

#endif
