#pragma once

#include <stdint.h>
#include "Task.h"
#include "Memory/PagingManager.h"
#include "AuxilaryVector.h"

class ELF
{
public:
    static void LoadELF(const String& path, PagingManager* pagingManager, uintptr_t& entry, AuxilaryVector*& auxilaryVector);
private:
    enum class ELFType : uint16_t;
    enum class ProgramHeaderType : uint32_t;

    struct ProgramHeader;
    struct ELFHeader;

    static void LoadProgramHeader(int elfFile, const ProgramHeader& programHeader,
                                  ELFHeader* elfHeader, PagingManager* pagingManager);
};

struct ELF::ProgramHeader
{
    ProgramHeaderType type;
    uint32_t flags;
    uint64_t offsetInFile;
    uint64_t virtAddr;
    uint64_t physAddr;
    uint64_t segmentSizeInFile;
    uint64_t segmentSizeInMemory;
    uint64_t align;
} __attribute__((packed));


struct ELF::ELFHeader
{
    uint8_t eIdentMagic[4];
    uint8_t eIdentClass; // 1 for 32-bit, 2 for 64-bit
    uint8_t eIdentEndianness;
    uint8_t eIdentVersion;
    uint8_t eIdentAbi;
    uint8_t eIdentAbiVersion;
    uint8_t eIdentZero[7];
    ELFType type;
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

enum class ELF::ELFType : uint16_t
{
    None = 0,
    Relocatable = 1,
    Executable = 2,
    Shared = 3,
    Core = 4
};

enum class ELF::ProgramHeaderType : uint32_t
{
    Null = 0,
    Load = 1,
    Dynamic = 2,
    Interpreter = 3,
    Note = 4,
    SharedLibrary = 5,
    ProgramHeaderTable = 6,
    ThreadLocalStorage = 7
};