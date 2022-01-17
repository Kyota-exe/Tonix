#ifndef MISKOS_EXT2INODE_H
#define MISKOS_EXT2INODE_H

#include <stdint.h>

namespace Ext2
{
    struct Ext2Inode
    {
        uint16_t typePermissions;
        uint16_t userId;
        uint32_t size0;
        uint32_t lastAccessTime;
        uint32_t creationTime;
        uint32_t lastModificationTime;
        uint32_t deletionTime;
        uint16_t groupId;
        uint16_t hardLinksCount;
        uint32_t diskSectorsCount;
        uint32_t flags;
        uint32_t reserved0; // Reserved by Linux, customizable
        uint32_t directBlockPointers[12];
        uint32_t singlyIndirectBlockPtr; // Points to a block that is a list of direct block pointers
        uint32_t doublyIndirectBlockPtr; // Points to a block that is a list of pointers to singly indirect block pointers
        uint32_t triplyIndirectBlockPtr; // Points to a block that is a list of pointers to doubly indirect block pointers
        uint32_t generationNumber;
        uint32_t fileACLBlockPtr;
        uint32_t size1; // If the file is a directory, this field is a pointer to a directory ACL
        uint32_t fragmentBlockAddr;
        uint8_t fragmentNumber; // Customizable
        uint8_t fragmentSize; // Customizable
        uint16_t reserved1; // Customizable
        uint16_t userId1; // Customizable
        uint16_t groupId1; // Customizable
        uint32_t reserved2; // Customizable
        uint64_t Read(void* buffer, uint64_t count, uint64_t readPos = 0);
        uint32_t GetBlockAddr(uint32_t currentBlock);
    } __attribute__((packed));
}

#endif
