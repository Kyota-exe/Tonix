#include "VFS.h"
#include "Memory/Memory.h"
#include "Ext2.h"
#include "StringUtilities.h"
#include "Panic.h"
#include "Vector.h"
#include "Serial.h"

VNode TraversePath(char* absolutePath)
{
    Ext2::Ext2Inode* currentInode = Ext2::rootDirInode;
    VNode lastVNode;

    char* currentElement = String::Split(absolutePath, '/', &absolutePath);
    KAssert(*currentElement == 0, "Absolute path does not begin at root.");

    uint64_t remainingPathDepth = String::Count(absolutePath, '/') + 1;
    while (remainingPathDepth > 0)
    {
        currentElement = String::Split(absolutePath, '/', &absolutePath);
        bool foundNextInode = false;
        for (const VNode& nodeInDirectory : Ext2::GetDirectoryListing(currentInode))
        {
            if (String::Equals(nodeInDirectory.name, currentElement))
            {
                foundNextInode = true;
                lastVNode = nodeInDirectory;
                currentInode = Ext2::GetInode(nodeInDirectory.inodeNum);
                break;
            }
        }
        remainingPathDepth--;

        // If the entry in the directory could not be found
        KAssert(foundNextInode, "Directory entry could not be found while traversing path.");
    }

    return VNode(lastVNode.name, lastVNode.inodeNum);
}

int Open(char* path, Process* process)
{
    // The VNode* in the descriptor will never be a dangling pointer since VNodes
    // stay in memory for the duration their file descriptor is alive.
    // TODO: Can't this memory stuff just be managed in the FileDescriptor constructor/destructor?
    FileDescriptor descriptor;
    descriptor.vNode = new VNode();
    *descriptor.vNode = TraversePath(path);
    int descriptorIndex = (int)process->fileDescriptors.GetLength();
    process->fileDescriptors.Push(descriptor);
    Serial::Print(descriptor.vNode->name);

    return descriptorIndex;
}

uint64_t Read(int descriptor, void* buffer, uint64_t count, Process* process)
{
    uint32_t inodeNum = process->fileDescriptors[descriptor].vNode->inodeNum;
    return Ext2::GetInode(inodeNum)->Read(buffer, count);
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