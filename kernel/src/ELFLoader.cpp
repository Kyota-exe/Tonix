#include "Memory/Memory.h"
#include "Memory/PageFrameAllocator.h"
#include "Memory/PagingManager.h"
#include "ELFLoader.h"
#include "Panic.h"
#include "VFS.h"

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
    uint16_t headerSize; // Normally 64 bytes for 64-bit and 52 bytes for 32-bit
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
} __attribute__((packed));

constexpr uint32_t PT_LOAD = 1;

uint64_t ELFLoader::LoadELF(int elfFile, PagingManager* pagingManager)
{
    auto elfHeader = new ELFHeader;
    uint64_t elfHeaderSize = Read(elfFile, elfHeader, sizeof(ELFHeader));
    KAssert(elfHeaderSize == sizeof(ELFHeader), "Invalid ELF file: failed to read header.");

    KAssert(elfHeader->eIdentMagic[0] == 0x7f &&
            elfHeader->eIdentMagic[1] == 0x45 &&
            elfHeader->eIdentMagic[2] == 0x4c &&
            elfHeader->eIdentMagic[3] == 0x46,
            "Invalid ELF file: magic bytes mismatch.");

    KAssert(elfHeader->programHeaderTableEntrySize == sizeof(ProgramHeader), "Invalid ELF file: invalid header.");

    uint64_t programHeaderTableSize = elfHeader->programHeaderTableEntryCount * elfHeader->programHeaderTableEntrySize;

    auto programHeaderTable = new ProgramHeader[elfHeader->programHeaderTableEntryCount];
    uint64_t programHeaderTableSizeRead = Read(elfFile, programHeaderTable, programHeaderTableSize);

    KAssert(programHeaderTableSize == programHeaderTableSizeRead, "Invalid ELF file: failed to read program header table.");

    for (uint16_t i = 0; i < elfHeader->programHeaderTableEntryCount; ++i)
    {
        ProgramHeader programHeader = programHeaderTable[i];
        if (programHeader.type == PT_LOAD)
        {
            uint64_t segmentPagesCount = programHeader.segmentSizeInMemory / 0x1000;
            if (programHeader.segmentSizeInMemory % 0x1000 != 0) segmentPagesCount++;

            for (uint64_t page = 0; page < segmentPagesCount; ++page)
            {
                void* physAddr = RequestPageFrame();
                void* virtAddr = (void*)(programHeader.virtAddr + page * 0x1000);
                pagingManager->MapMemory(virtAddr, physAddr, true);
                void* upperHalfVirtAddr = (void*)((uint64_t)physAddr + 0xffff'8000'0000'0000);

                RepositionOffset(elfFile, programHeader.offsetInFile, VFSSeekType::Set);
                Read(elfFile, upperHalfVirtAddr, programHeader.segmentSizeInFile);

                uint64_t paddingSize = programHeader.segmentSizeInMemory - programHeader.segmentSizeInFile;
                Memset((void*)((uint64_t)virtAddr + programHeader.segmentSizeInFile), 0, paddingSize);
            }
        }
    }

    return elfHeader->entry;
}