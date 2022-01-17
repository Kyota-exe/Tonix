#include "Memory/Memory.h"
#include "VFS.h"
#include "Scheduler.h"
#include "Ext2.h"
#include "StringUtilities.h"
#include "Serial.h"
#include "Panic.h"

namespace VFS
{
    VNode TraversePath(char* absolutePath)
    {
        /*Ext2::Ext2Inode* currentInode = Ext2::rootDirInode;
        char* currentElement = nullptr;
        while
        {
            currentElement = String::Split(absolutePath, '/', &absolutePath);
            Serial::Print(currentElement);

            for (const VNode& nodeInDirectory : GetDirectoryListing(currentInode))
            {
                if (String::Equals(nodeInDirectory.name, currentElement))
                {
                    currentInode = Ext2::GetInode(nodeInDirectory.inodeNum);
                    break;
                }

                // If the entry in the directory could not be found
                Serial::Print("Invalid path");
                Serial::Print("Hanging...");
                while (true) asm("hlt");
            }
        }*/

        Ext2::Ext2Inode* currentInode = Ext2::rootDirInode;
        VNode* lastVNode = new VNode();

        char* currentElement = String::Split(absolutePath, '/', &absolutePath);
        KAssert(*currentElement == 0, "Absolute path does not begin at root.");

        Serial::Print("absolute: ", "");
        Serial::Print(absolutePath);
        while (*(currentElement = String::Split(absolutePath, '/', &absolutePath)) != 0)
        {
            Serial::Print(currentElement);
            for (VNode nodeInDirectory : Ext2::GetDirectoryListing(currentInode))
            {
                Serial::Print("--------------------- fond: ", "");
                Serial::Print(nodeInDirectory.name);
                if (String::Equals(nodeInDirectory.name, currentElement))
                {
                    *lastVNode = nodeInDirectory;
                    currentInode = Ext2::GetInode(nodeInDirectory.inodeNum);
                    break;
                }
            }

            // If the entry in the directory could not be found
            Panic("Directory entry could not be found while traversing path.");
        }

        return {lastVNode->name, lastVNode->inodeNum};
    }

    int Open(char* path)
    {
        Task* currentTask = &(*taskList)[currentTaskIndex];

        // The VNode* in the descriptor will never be a dangling pointer since VNodes
        // stay in memory for the duration their file descriptor is alive.
        // TODO: Can't this memory stuff just be managed in the FileDescriptor constructor/destructor?
        FileDescriptor descriptor;
        descriptor.vNode = new VNode();
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
        name = new char[nameLength + 1];
        MemCopy(name, (void*)_name, nameLength);
        name[nameLength] = 0;
    }

    VNode::VNode(const VNode &original) : inodeNum(original.inodeNum)
    {
        uint64_t nameLength = String::Length(original.name);
        name = new char[nameLength + 1];
        MemCopy(name, original.name, nameLength);
        name[nameLength] = 0;
    }

    VNode::~VNode()
    {
        delete[] name;
    }

    VNode& VNode::operator=(const VNode& newValue)
    {
        if (&newValue != this)
        {
            delete[] name;
            uint64_t nameLength = String::Length(newValue.name);
            name = new char[nameLength + 1];
            MemCopy(name, newValue.name, nameLength);
            name[nameLength] = 0;

            inodeNum = newValue.inodeNum;
        }

        return *this;
    }
}