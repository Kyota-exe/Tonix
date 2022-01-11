#include "Ext2Inode.h"
#include "Serial.h"

uint32_t Ext2Inode::GetBlock(uint32_t currentBlockIndex, uint32_t blockSize)
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