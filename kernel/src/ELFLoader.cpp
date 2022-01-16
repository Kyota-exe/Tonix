#include "Memory/Memory.h"
#include "Memory/PageFrameAllocator.h"
#include "Memory/PagingManager.h"
#include "ELFLoader.h"
#include "Serial.h"

const uint32_t PT_LOAD = 1;

struct ELFHeader
{
    uint8_t eIdentMagic[4];
    uint8_t eIdentClass; // 1 for 32-bit, 2 for 64-bit
    uint8_t eIdentEndianness;
    uint8_t eIdentVersion;
    uint8_t eIdentAbi;
    uint8_t eIdentAbiVersion;
    uint8_t eIdentZero[7];
    uint16_t type;
    uint16_t architecture;
    uint32_t version;
    uint64_t entry;
    uint64_t programHeaderTableOffset;
    uint64_t sectionHeaderTableOffset;
    uint32_t flags;
    uint16_t headerSize; // Normally 64B for 64-bit and 52B for 32-bit
    uint16_t programHeaderTableEntrySize;
    uint16_t programHeaderTableEntryCount;
    uint16_t sectionHeaderTableEntrySize;
    uint16_t sectionHeaderTableEntryCount;
    uint16_t sectionNamesEntryIndex;
} __attribute__((packed));

struct ProgramHeader
{
    uint32_t type;
    uint32_t flags;
    uint64_t offsetInFile;
    uint64_t virtAddr;
    uint64_t physAddr;
    uint64_t segmentSizeInFile;
    uint64_t segmentSizeInMemory;
    uint64_t align;
};

extern "C" void SwitchToRing3(uint64_t entry, uint64_t stackBase, char in, uint64_t printAddr);

uint64_t LoadELF(uint64_t ramDiskBegin, PagingManager* pagingManager)
{
    // TODO: read from disk
    auto elfHeader = (ELFHeader*)ramDiskBegin;

    if (elfHeader->eIdentMagic[0] != 0x7f || elfHeader->eIdentMagic[1] != 0x45 || elfHeader->eIdentMagic[2] != 0x4c || elfHeader->eIdentMagic[3] != 0x46)
    {
        Serial::Print("Magic bytes do not match.");
        Serial::Print("Hanging...");
        while (true) asm("hlt");
    }

    if (elfHeader->programHeaderTableEntrySize != sizeof(ProgramHeader))
    {
        Serial::Printf("Program header table entry size from the ELF header field (%x)", elfHeader->programHeaderTableEntrySize, "");
        Serial::Printf(" does not match with the hardcoded program header table entry size (%x).", sizeof(ProgramHeader));
    }

    // TODO: read from disk
    auto programHeaderTable = (ProgramHeader*)(ramDiskBegin + elfHeader->programHeaderTableOffset);

    for (uint16_t i = 0; i < elfHeader->programHeaderTableEntryCount; ++i)
    {
        ProgramHeader programHeader = programHeaderTable[i];
        if (programHeader.type == PT_LOAD)
        {
            uint64_t segmentPagesCount = programHeader.segmentSizeInMemory / 0x1000 + programHeader.segmentSizeInMemory % 0x1000 != 0 ? 1 : 0;
            for (uint64_t page = 0; page < segmentPagesCount; ++page)
            {
                void* physAddr = RequestPageFrame();
                void* virtAddr = (void*)(programHeader.virtAddr + page * 0x1000);

                pagingManager->MapMemory(virtAddr, physAddr);

                // TODO: read from disk
                void* upperHalfVirtAddr = (void*)((uint64_t)physAddr + 0xffff'8000'0000'0000);
                MemCopy(upperHalfVirtAddr, (void*)(ramDiskBegin + programHeader.offsetInFile), programHeader.segmentSizeInFile);

                uint64_t paddingSize = programHeader.segmentSizeInMemory - programHeader.segmentSizeInFile;
                Memset((void*)((uint64_t)virtAddr + programHeader.segmentSizeInFile), 0, paddingSize);
            }
        }
    }

    return elfHeader->entry;
}