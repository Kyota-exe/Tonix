#include "Memory/Memory.h"
#include "Memory/PageFrameAllocator.h"
#include "Memory/PagingManager.h"
#include "ELF.h"
#include "VFS.h"
#include "Serial.h"

constexpr uintptr_t USER_STACK_BASE = 0x0000'8000'0000'0000 - 0x1000;
constexpr uintptr_t USER_STACK_SIZE = 0x20000;

constexpr uintptr_t RTDL_ADDR = 0x40000000;

void ELF::LoadELF(const String& path, PagingManager* pagingManager, uintptr_t& entry, uintptr_t& stackPtr)
{
    int elfFile = VFS::kernelVfs->Open(path, VFS::OpenFlag::ReadOnly);

    auto elfHeader = new ELFHeader;
    VFS::kernelVfs->Read(elfFile, elfHeader, sizeof(ELFHeader));

    Assert(elfHeader->eIdentMagic[0] == 0x7f &&
           elfHeader->eIdentMagic[1] == 0x45 &&
           elfHeader->eIdentMagic[2] == 0x4c &&
           elfHeader->eIdentMagic[3] == 0x46);

    Assert(elfHeader->programHeaderTableEntrySize == sizeof(ProgramHeader));
    Assert(elfHeader->type == ELFType::Executable || elfHeader->type == ELFType::Shared);

    uint64_t programHeaderTableSize = elfHeader->programHeaderTableEntryCount * elfHeader->programHeaderTableEntrySize;
    auto programHeaderTable = new ProgramHeader[elfHeader->programHeaderTableEntryCount];
    VFS::kernelVfs->Read(elfFile, programHeaderTable, programHeaderTableSize);

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

                VFS::kernelVfs->RepositionOffset(elfFile, programHeader.offsetInFile, VFS::SeekType::Set);
                VFS::kernelVfs->Read(elfFile, rtdlPath, programHeader.segmentSizeInFile);

                rtdlPath[programHeader.segmentSizeInFile] = 0;

                LoadELF(String(rtdlPath), pagingManager, entry, stackPtr);
                delete[] rtdlPath;

                hasDynamicLinking = true;
                break;
            }
            default: {}
        }
    }

    if (elfHeader->type == ELFType::Shared)
    {
        entry = RTDL_ADDR + elfHeader->entry;
    }
    else if (elfHeader->type == ELFType::Executable)
    {
        Assert(USER_STACK_SIZE % 0x1000 == 0);
        uint64_t stackPageCount = USER_STACK_SIZE / 0x1000;
        uintptr_t stackLowestVirtAddr = USER_STACK_BASE - USER_STACK_SIZE;

        uintptr_t stackPhysAddr {};
        for (uint64_t pageIndex = 0; pageIndex < stackPageCount; ++pageIndex)
        {
            auto virtAddr = reinterpret_cast<void*>(stackLowestVirtAddr + pageIndex * 0x1000);
            stackPhysAddr = RequestPageFrame();
            pagingManager->MapMemory(virtAddr, reinterpret_cast<void*>(stackPhysAddr));
        }

        uintptr_t stackHigherHalfAddr = HigherHalf(stackPhysAddr + 0x1000);
        auto stackHigherHalf = reinterpret_cast<uintptr_t*>(stackHigherHalfAddr);

        if (hasDynamicLinking)
        {
            auto push = [stackHigherHalfAddr, &stackHigherHalf] (uint64_t value)
            {
                uintptr_t usedStackSize = stackHigherHalfAddr - reinterpret_cast<uintptr_t>(stackHigherHalf);
                Assert(usedStackSize + sizeof(value) <= 0x1000);
                *--stackHigherHalf = value;
            };

            // Auxiliary vector
            push(0); // NULL
            push(0);
            push(programHeaderTableAddr);
            push(3);
            push(elfHeader->programHeaderTableEntrySize);
            push(4);
            push(elfHeader->programHeaderTableEntryCount);
            push(5);
            push(elfHeader->entry);
            push(9);

            // Environment
            push(0); // NULL

            // Argument vector (argv)
            push(0); // NULL

            // Argument count (argc)
            push(0);
        }
        else
        {
            entry = elfHeader->entry;
        }

        uintptr_t usedStackSize = stackHigherHalfAddr - reinterpret_cast<uintptr_t>(stackHigherHalf);
        stackPtr = USER_STACK_BASE - usedStackSize;
    }

    delete elfHeader;
    delete[] programHeaderTable;

    VFS::kernelVfs->Close(elfFile);
}

void ELF::LoadProgramHeader(int elfFile, const ProgramHeader& programHeader,
                            ELFHeader* elfHeader, PagingManager* pagingManager)
{
    Assert(programHeader.type == ProgramHeaderType::Load);

    uintptr_t baseAddr = programHeader.virtAddr;
    if (elfHeader->type == ELFType::Shared) baseAddr += RTDL_ADDR;
    uintptr_t basePageAddr = baseAddr - (baseAddr % 0x1000);

    VFS::kernelVfs->RepositionOffset(elfFile, programHeader.offsetInFile, VFS::SeekType::Set);

    uint64_t readCount = 0;
    uint64_t segmentPagesCount = (programHeader.segmentSizeInMemory - 1) / 0x1000 + 1;
    for (uint64_t pageIndex = 0; pageIndex < segmentPagesCount; ++pageIndex)
    {
        uintptr_t physAddr = RequestPageFrame();
        uintptr_t virtAddr = basePageAddr + pageIndex * 0x1000;
        pagingManager->MapMemory(reinterpret_cast<void*>(virtAddr), reinterpret_cast<void*>(physAddr));

        uintptr_t higherHalfAddr = HigherHalf(physAddr);
        Memset(reinterpret_cast<void*>(higherHalfAddr), 0, 0x1000);

        uint64_t count = 0x1000;
        if (pageIndex == 0)
        {
            higherHalfAddr += baseAddr % 0x1000;
            count -= baseAddr % 0x1000;
        }

        if (readCount + count > programHeader.segmentSizeInFile)
        {
            count = programHeader.segmentSizeInFile - readCount;
        }

        VFS::kernelVfs->Read(elfFile, reinterpret_cast<void*>(higherHalfAddr), count);
        readCount += count;
    }

    Assert(readCount == programHeader.segmentSizeInFile);
}