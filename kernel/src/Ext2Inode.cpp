#include "Ext2Inode.h"
#include "Memory/Memory.h"
#include "Serial.h"
#include "Ext2.h"

namespace Ext2
{
    uint64_t Ext2Inode::Read(void* buffer, uint64_t count, uint64_t readPos)
    {
        // TODO: Support sizes bigger than 2^32 bytes
        if (readPos + count > size0)
        {
            count = size0 - readPos;
        }

        uint64_t currentReadPos = readPos;
        uint64_t parsedCount = 0;
        while (parsedCount < count)
        {
            uint64_t remainingBytes = (count - parsedCount);
            uint64_t offsetInBlock = (currentReadPos % blockSize);
            uint64_t memCopySize = blockSize - offsetInBlock;
            if (memCopySize > remainingBytes) memCopySize = remainingBytes;

            uint64_t diskAddr = GetBlockAddr(currentReadPos / blockSize) * blockSize + offsetInBlock;
            MemCopy(buffer, (void*)(diskAddr + ramDiskAddr), memCopySize);

            parsedCount += memCopySize;
        }

        return parsedCount;
    }

    uint32_t Ext2Inode::GetBlockAddr(uint32_t currentBlockIndex)
    {
        uint64_t pointersPerBlock = blockSize / sizeof(uint32_t);

        if (currentBlockIndex < 12)
        {
            return directBlockPointers[currentBlockIndex];
        }

        if (currentBlockIndex < 12 + pointersPerBlock)
        {
            Serial::Print("Singly indirect pointers are not yet supported.");
            Serial::Print("Hanging...");
            while (true) asm("hlt");
        }

        if (currentBlockIndex < 12 + pointersPerBlock * pointersPerBlock)
        {
            Serial::Print("Doubly indirect pointers are not yet supported.");
            Serial::Print("Hanging...");
            while (true) asm("hlt");
        }

        if (currentBlockIndex < 12 + pointersPerBlock * pointersPerBlock * pointersPerBlock)
        {
            Serial::Print("Triply indirect pointers are not yet supported.");
            Serial::Print("Hanging...");
            while (true) asm("hlt");
        }

        Serial::Printf("Unsupported file contents block index (%d).", currentBlockIndex);
        Serial::Print("Hanging...");
        while (true) asm("hlt");
    }
}