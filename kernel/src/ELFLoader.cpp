#include "Memory/Memory.h"
#include "Memory/PageFrameAllocator.h"
#include "Memory/PagingManager.h"
#include "ELFLoader.h"
#include "Panic.h"
#include "VFS.h"
#include "Serial.h"
#include "ELF.h"

constexpr uint64_t RTDL_ADDR = 0x40000000;

uint64_t ELFLoader::LoadELF(const String& path, Process* process)
{
    PagingManager* pagingManager = process->pagingManager;

    Error error;
    int elfFile = Open(path, 0, error);
    KAssert(elfFile != -1, "Failed to read ELF file. Error code: %d", error);

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

    Serial::Printf("Program header count: %d", elfHeader->programHeaderTableEntryCount);
    for (uint16_t i = 0; i < elfHeader->programHeaderTableEntryCount; ++i)
    {
        ProgramHeader programHeader = programHeaderTable[i];
        Serial::Print("Program header---------------------------------------------------");
        Serial::Printf("Type: %d", programHeader.type);

        if (programHeader.type == ProgramHeaderType::Load)
        {
            Serial::Printf("Virtual address: %x", programHeader.virtAddr);
            Serial::Printf("Memory size: %x", programHeader.segmentSizeInMemory);

            uint64_t baseAddr = programHeader.virtAddr;

            // If the virtual address is 0, this ELF is a RTDL
            if (programHeader.virtAddr == 0)
            {
                baseAddr = RTDL_ADDR;
            }

            uint64_t segmentPagesCount = (programHeader.segmentSizeInMemory - 1) / 0x1000 + 1;
            Serial::Printf("Segment page count: %d", segmentPagesCount);

            RepositionOffset(elfFile, programHeader.offsetInFile, VFSSeekType::Set);

            for (uint64_t page = 0; page < segmentPagesCount; ++page)
            {
                void* physAddr = RequestPageFrame();
                void* virtAddr = (void*)(baseAddr + page * 0x1000);

                pagingManager->MapMemory(virtAddr, physAddr, true);

                void* higherHalfVirtAddr = (void*)((uint64_t)physAddr + 0xffff'8000'0000'0000);

                Memset(higherHalfVirtAddr, 0, 0x1000);
                Read(elfFile, higherHalfVirtAddr, 0x1000);
            }
        }
        else if (programHeader.type == ProgramHeaderType::Interpreter)
        {
            char* rtdlPath = new char[programHeader.segmentSizeInFile + 1];

            RepositionOffset(elfFile, programHeader.offsetInFile, VFSSeekType::Set);
            Read(elfFile, rtdlPath, programHeader.segmentSizeInFile);

            rtdlPath[programHeader.segmentSizeInFile] = 0;

            LoadELF(String(rtdlPath), process);
        }
    }

    Close(elfFile);
    return elfHeader->entry;
}