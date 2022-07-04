#include "Memory/Memory.h"
#include "Memory/PageFrameAllocator.h"
#include "Memory/PagingManager.h"
#include "ELF.h"
#include "VFS.h"
#include "Serial.h"

constexpr uintptr_t RTDL_ADDR = 0x40000000;

void ELF::LoadELF(const String& path, PagingManager& pagingManager, VFS& vfs, uintptr_t& entry, AuxiliaryVector*& auxiliaryVector)
{
    int elfFile = vfs.Open(path, VFS::OpenFlag::ReadOnly);

    auto elfHeader = new ELFHeader;
    vfs.Read(elfFile, elfHeader, sizeof(ELFHeader));

    Assert(elfHeader->eIdentMagic[0] == 0x7f &&
           elfHeader->eIdentMagic[1] == 0x45 &&
           elfHeader->eIdentMagic[2] == 0x4c &&
           elfHeader->eIdentMagic[3] == 0x46);

    Assert(elfHeader->programHeaderTableEntrySize == sizeof(ProgramHeader));
    Assert(elfHeader->type == ELFType::Executable || elfHeader->type == ELFType::Shared);

    uint64_t programHeaderTableSize = elfHeader->programHeaderTableEntryCount * elfHeader->programHeaderTableEntrySize;
    auto programHeaderTable = new ProgramHeader[elfHeader->programHeaderTableEntryCount];
    vfs.Read(elfFile, programHeaderTable, programHeaderTableSize);

    bool hasDynamicLinking = false;
    uintptr_t programHeaderTableAddr = 0;

    for (uint16_t i = 0; i < elfHeader->programHeaderTableEntryCount; ++i)
    {
        ProgramHeader programHeader = programHeaderTable[i];

        switch (programHeader.type)
        {
            case ProgramHeaderType::Load:
            {
                LoadProgramHeader(elfFile, programHeader, elfHeader, pagingManager, vfs);
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

                vfs.RepositionOffset(elfFile, programHeader.offsetInFile, VFS::SeekType::Set);
                vfs.Read(elfFile, rtdlPath, programHeader.segmentSizeInFile);

                rtdlPath[programHeader.segmentSizeInFile] = 0;

                AuxiliaryVector* auxVector = nullptr;
                LoadELF(String(rtdlPath), pagingManager, vfs, entry, auxVector);
                Assert(auxVector == nullptr);

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
    else if (elfHeader->type == ELFType::Executable && !hasDynamicLinking)
    {
        entry = elfHeader->entry;
    }

    if (hasDynamicLinking)
    {
        auxiliaryVector = new AuxiliaryVector();
        auxiliaryVector->entry = elfHeader->entry;
        auxiliaryVector->programHeaderTableAddr = programHeaderTableAddr;
        auxiliaryVector->programHeaderTableEntrySize = elfHeader->programHeaderTableEntrySize;
        auxiliaryVector->programHeaderTableEntryCount = elfHeader->programHeaderTableEntryCount;
    }
    else auxiliaryVector = nullptr;

    delete elfHeader;
    delete[] programHeaderTable;

    vfs.Close(elfFile);
}

void ELF::LoadProgramHeader(int elfFile, const ProgramHeader& programHeader,
                            ELFHeader* elfHeader, PagingManager& pagingManager, VFS& vfs)
{
    Assert(programHeader.type == ProgramHeaderType::Load);

    uintptr_t baseAddr = programHeader.virtAddr;
    if (elfHeader->type == ELFType::Shared) baseAddr += RTDL_ADDR;
    uintptr_t basePageAddr = baseAddr - (baseAddr % 0x1000);

    vfs.RepositionOffset(elfFile, programHeader.offsetInFile, VFS::SeekType::Set);

    uint64_t readCount = 0;
    uint64_t segmentPagesCount = (programHeader.segmentSizeInMemory - 1) / 0x1000 + 1;
    for (uint64_t pageIndex = 0; pageIndex < segmentPagesCount; ++pageIndex)
    {
        uintptr_t physAddr = RequestPageFrame();
        uintptr_t virtAddr = basePageAddr + pageIndex * 0x1000;
        pagingManager.MapMemory(reinterpret_cast<void*>(virtAddr), reinterpret_cast<void*>(physAddr));

        uintptr_t higherHalfAddr = HigherHalf(physAddr);
        memset(reinterpret_cast<void*>(higherHalfAddr), 0, 0x1000);

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

        vfs.Read(elfFile, reinterpret_cast<void*>(higherHalfAddr), count);
        readCount += count;
    }

    Assert(readCount == programHeader.segmentSizeInFile);
}
