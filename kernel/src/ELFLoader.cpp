#include "Memory/Memory.h"
#include "Memory/PageFrameAllocator.h"
#include "Memory/PagingManager.h"
#include "ELFLoader.h"
#include "Panic.h"
#include "VFS.h"
#include "Serial.h"
#include "ELF.h"
#include "SegmentSelectors.h"

constexpr uintptr_t USER_STACK_BASE = 0x0000'8000'0000'0000 - 0x1000;
constexpr uintptr_t USER_STACK_SIZE = 0x2000;

constexpr uintptr_t RTDL_ADDR = 0x40000000;

void ELFLoader::LoadELF(const String& path, Process* process)
{
    PagingManager* pagingManager = process->pagingManager;

    Error elfFileError;
    int elfFile = VFS::Open(path, 0, elfFileError);
    KAssert(elfFile != -1, "Failed to read ELF file. Error code: %d", elfFileError);

    auto elfHeader = new ELFHeader;
    uint64_t elfHeaderSize = VFS::Read(elfFile, elfHeader, sizeof(ELFHeader));
    KAssert(elfHeaderSize == sizeof(ELFHeader), "Invalid ELF file: failed to read header.");

    KAssert(elfHeader->eIdentMagic[0] == 0x7f &&
            elfHeader->eIdentMagic[1] == 0x45 &&
            elfHeader->eIdentMagic[2] == 0x4c &&
            elfHeader->eIdentMagic[3] == 0x46,
            "Invalid ELF file: magic bytes mismatch.");

    KAssert(elfHeader->programHeaderTableEntrySize == sizeof(ProgramHeader), "Invalid ELF file: invalid header.");
    KAssert(elfHeader->type == ELFType::Executable || elfHeader->type == ELFType::Shared, "Unsupported ELF type.");

    uint64_t programHeaderTableSize = elfHeader->programHeaderTableEntryCount * elfHeader->programHeaderTableEntrySize;
    auto programHeaderTable = new ProgramHeader[elfHeader->programHeaderTableEntryCount];

    uint64_t programHeaderTableSizeRead = VFS::Read(elfFile, programHeaderTable, programHeaderTableSize);
    KAssert(programHeaderTableSize == programHeaderTableSizeRead,
            "Invalid ELF file: failed to read program header table.");

    bool hasDynamicLinking = false;
    uintptr_t programHeaderTableAddr = 0;

    for (uint16_t i = 0; i < elfHeader->programHeaderTableEntryCount; ++i)
    {
        ProgramHeader programHeader = programHeaderTable[i];

        switch (programHeader.type)
        {
            case ProgramHeaderType::Load:
            {
                LoadProgramHeader(elfFile, programHeader, elfHeader, pagingManager);
                break;
            }
            case ProgramHeaderType::ProgramHeaderTable:
            {
                programHeaderTableAddr = programHeader.virtAddr;
                break;
            }
            case ProgramHeaderType::Interpreter:
            {
                char* rtdlPath = new char[programHeader.segmentSizeInFile + 1];

                Error error;
                VFS::RepositionOffset(elfFile, programHeader.offsetInFile, VFS::SeekType::Set, error);
                VFS::Read(elfFile, rtdlPath, programHeader.segmentSizeInFile);

                rtdlPath[programHeader.segmentSizeInFile] = 0;

                LoadELF(String(rtdlPath), process);

                hasDynamicLinking = true;
                break;
            }
            default: {}
        }
    }

    if (elfHeader->type == ELFType::Shared)
    {
        process->frame.rip = RTDL_ADDR + elfHeader->entry;
    }
    else if (elfHeader->type == ELFType::Executable)
    {
        process->frame.cs = USER_CODE_SEGMENT;
        process->frame.ss = USER_DATA_SEGMENT;
        process->frame.ds = USER_DATA_SEGMENT;
        process->frame.es = USER_DATA_SEGMENT;
        process->frame.rflags = USER_INITIAL_RFLAGS;

        uint64_t stackPageCount = USER_STACK_SIZE / 0x1000;
        uintptr_t stackLowestVirtAddr = USER_STACK_BASE - USER_STACK_SIZE;
        auto stackLowestPhysAddr = reinterpret_cast<uintptr_t>(RequestPageFrames(stackPageCount));
        void* stackPhysAddr = nullptr;
        for (uint64_t page = 0; page < USER_STACK_SIZE; page += 0x1000)
        {
            stackPhysAddr = reinterpret_cast<void*>(stackLowestPhysAddr + page);
            auto virtAddr = reinterpret_cast<void*>(stackLowestVirtAddr + page);
            pagingManager->MapMemory(virtAddr, stackPhysAddr, true);
        }

        uintptr_t stackHigherHalfAddr = HigherHalf(reinterpret_cast<uintptr_t>(stackPhysAddr)) + 0x1000;
        auto stackHigherHalf = reinterpret_cast<uintptr_t*>(stackHigherHalfAddr);

        if (hasDynamicLinking)
        {
            // Auxiliary vector
            *--stackHigherHalf = 0; // NULL
            *--stackHigherHalf = 0;
            *--stackHigherHalf = programHeaderTableAddr;
            *--stackHigherHalf = 3;
            *--stackHigherHalf = elfHeader->programHeaderTableEntrySize;
            *--stackHigherHalf = 4;
            *--stackHigherHalf = elfHeader->programHeaderTableEntryCount;
            *--stackHigherHalf = 5;
            *--stackHigherHalf = elfHeader->entry;
            *--stackHigherHalf = 9;

            // Environment
            *--stackHigherHalf = 0;

            // Argument vector (argv)
            *--stackHigherHalf = 0; // NULL

            // Argument count (argc)
            *--stackHigherHalf = 0;
        }
        else
        {
            process->frame.rip = elfHeader->entry;
        }

        process->frame.rsp = USER_STACK_BASE - (stackHigherHalfAddr - reinterpret_cast<uintptr_t>(stackHigherHalf));
    }

    delete elfHeader;
    delete[] programHeaderTable;

    VFS::Close(elfFile);
}

void ELFLoader::LoadProgramHeader(int elfFile, const ProgramHeader& programHeader,
                                  ELFHeader* elfHeader, PagingManager* pagingManager)
{
    uint64_t segmentPagesCount = (programHeader.segmentSizeInMemory - 1) / 0x1000 + 1;

    Error error;
    VFS::RepositionOffset(elfFile, programHeader.offsetInFile, VFS::SeekType::Set, error);

    uintptr_t baseAddr = programHeader.virtAddr;
    if (elfHeader->type == ELFType::Shared) baseAddr += RTDL_ADDR;

    uint64_t fileReadCount = 0;
    for (uint64_t page = 0; page < segmentPagesCount; ++page)
    {
        auto physAddr = RequestPageFrame();
        auto virtAddr = reinterpret_cast<void*>(baseAddr + (page * 0x1000));

        void* pageAddr = (void*)((uintptr_t)virtAddr - ((uintptr_t)virtAddr % 0x1000));
        pagingManager->MapMemory(pageAddr, physAddr, true);

        auto higherHalfAddr = HigherHalf(reinterpret_cast<uintptr_t>(physAddr));
        Memset(reinterpret_cast<void*>(higherHalfAddr), 0, 0x1000);

        higherHalfAddr += baseAddr % 0x1000;

        uint64_t readCount = 0x1000;
        if (fileReadCount + readCount > programHeader.segmentSizeInFile)
        {
            readCount = programHeader.segmentSizeInFile % 0x1000;
        }

        fileReadCount += VFS::Read(elfFile, reinterpret_cast<void*>(higherHalfAddr), readCount);
    }
}