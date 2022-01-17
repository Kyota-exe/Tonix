#include "Memory/Memory.h"
#include "VFS.h"
#include "Scheduler.h"
#include "Ext2.h"
#include "StringUtilities.h"
#include "Serial.h"

namespace VFS
{
    VNode TraversePath(const char* path)
    {
        uint64_t deepestDepth = String::Count(path, '/');
        uint64_t currentDepth = 0;
        Ext2::Ext2Inode* currentInode = Ext2::rootDirInode;
        while (true)
        {
            const char* currentElement = String::Split(path, currentDepth, '/');
            if (currentElement == nullptr)
            {
                Serial::Print("Invalid path");
                Serial::Print("Hanging...");
                while (true) asm("hlt");
            }

            for (const VNode& nodeInDirectory : GetDirectoryListing(currentInode))
            {
                if (String::Equals(nodeInDirectory.name, currentElement))
                {
                    currentInode = Ext2::GetInode(nodeInDirectory.inodeNum);
                    currentDepth++;

                    if (deepestDepth == currentDepth)
                    {
                        return {nodeInDirectory.name, nodeInDirectory.inodeNum};
                    }
                    continue;
                }

                // If the entry in the directory could not be found
                Serial::Print("Invalid path");
                Serial::Print("Hanging...");
                while (true) asm("hlt");
            }
        }
    }

    int Open(const char* path)
    {
        Task* currentTask = &(*taskList)[currentTaskIndex];

        // The VNode* in the descriptor will never be a dangling pointer since VNodes
        // stay in memory for the duration their file descriptor is alive.
        FileDescriptor descriptor;
        descriptor.vNode = (VNode*)KMalloc(sizeof(VNode));
        *descriptor.vNode = TraversePath(path);
        int descriptorIndex = (int)currentTask->fileDescriptors.GetLength();
        currentTask->fileDescriptors.Push(descriptor);

        return descriptorIndex;
    }

    uint64_t Read(int descriptor, void* buffer, uint64_t count)
    {
        VNode* vNode = (*taskList)[currentTaskIndex].fileDescriptors[descriptor].vNode;
        return Ext2::GetInode(vNode->inodeNum)->Read(buffer, count);
    }

    VNode::VNode(const char* _name, uint32_t _inodeNum) : inodeNum(_inodeNum)
    {
        uint64_t nameLength = String::Length(_name);
        name = (char*)KMalloc(nameLength + 1);
        MemCopy(name, (void*)_name, nameLength);
        name[nameLength] = 0;

        children = Vector<VNode>();
    }

    VFS::VNode::VNode(const VNode &original) : inodeNum(original.inodeNum), children(original.children)
    {
        uint64_t nameLength = String::Length(original.name);
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
            uint64_t nameLength = String::Length(newValue.name);
            name = (char*)KMalloc(nameLength + 1);
            MemCopy(name, newValue.name, nameLength);
            name[nameLength] = 0;

            inodeNum = newValue.inodeNum;
            children = newValue.children;
        }

        return *this;
    }
}