#include "Memory/Memory.h"
#include "Memory/PageFrameAllocator.h"
#include "Memory/PagingManager.h"
#include "ELFLoader.h"
#include "Panic.h"
#include "VFS.h"
#include "Serial.h"
#include "ELF.h"

constexpr uint64_t RTDL_ADDR = 0x40000000;

constexpr uint16_t USER_CODE_SEGMENT = 0b01'0'11;
constexpr uint16_t USER_DATA_SEGMENT = 0b10'0'11;
constexpr uint64_t USER_INITIAL_RFLAGS = 0b1000000010;

const uint64_t USER_STACK_BASE = 0x0000'8000'0000'0000 - 0x1000;

void ELFLoader::LoadELF(const String& path, Process* process)
{
    PagingManager* pagingManager = process->pagingManager;

    Error elfFileError;
    int elfFile = Open(path, 0, elfFileError);
    KAssert(elfFile != -1, "Failed to read ELF file. Error code: %d", elfFileError);

    auto elfHeader = new ELFHeader;
    uint64_t elfHeaderSize = Read(elfFile, elfHeader, sizeof(ELFHeader));
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

    uint64_t programHeaderTableSizeRead = Read(elfFile, programHeaderTable, programHeaderTableSize);

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
                char *rtdlPath = new char[programHeader.segmentSizeInFile + 1];

                RepositionOffset(elfFile, programHeader.offsetInFile, VFSSeekType::Set);
                Read(elfFile, rtdlPath, programHeader.segmentSizeInFile);

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

		void* stackPageFramePhysAddr = RequestPageFrame();
        pagingManager->MapMemory(reinterpret_cast<void*>(USER_STACK_BASE - 0x1000), stackPageFramePhysAddr, true);

		uintptr_t stackHigherHalfAddr = HigherHalf(reinterpret_cast<uintptr_t>(stackPageFramePhysAddr)) + 0x1000;
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

            // Environment (currently null)
            *--stackHigherHalf = 0;

            // Argument vector (argv)
            *--stackHigherHalf = reinterpret_cast<uintptr_t>("program name");

            // Argument count (argc)
            *--stackHigherHalf = 1;
        }
		else
		{
            process->frame.rip = elfHeader->entry;
        }

		process->frame.rsp = USER_STACK_BASE - (stackHigherHalfAddr - reinterpret_cast<uintptr_t>(stackHigherHalf));
    }

    delete elfHeader;
    delete[] programHeaderTable;

    Close(elfFile);
}

void ELFLoader::LoadProgramHeader(int elfFile, const ProgramHeader& programHeader,
                                  ELFHeader* elfHeader, PagingManager* pagingManager)
{
    uint64_t segmentPagesCount = (programHeader.segmentSizeInMemory - 1) / 0x1000 + 1;

    RepositionOffset(elfFile, programHeader.offsetInFile, VFSSeekType::Set);

    for (uint64_t page = 0; page < segmentPagesCount; ++page)
    {
        uint64_t baseAddr = programHeader.virtAddr;
        if (elfHeader->type == ELFType::Shared) baseAddr += RTDL_ADDR;
        void* virtAddr = reinterpret_cast<void*>(baseAddr + page * 0x1000);

        auto physAddr = reinterpret_cast<uintptr_t>(RequestPageFrame());
        pagingManager->MapMemory(virtAddr, reinterpret_cast<void*>(physAddr), true);

        void* higherHalfVirtAddr = reinterpret_cast<void*>(HigherHalf(physAddr));

        Memset(higherHalfVirtAddr, 0, 0x1000);
        Read(elfFile, higherHalfVirtAddr, 0x1000);
    }
}