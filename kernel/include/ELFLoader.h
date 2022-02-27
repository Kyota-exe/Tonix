#pragma once

#include <stdint.h>
#include "Task.h"

class PagingManager;
struct ELFHeader;
struct ProgramHeader;

class ELFLoader
{
public:
    static void LoadELF(const String& path, PagingManager* pagingManager, uintptr_t& entry, uintptr_t& stackPtr);
private:
    static void LoadProgramHeader(int elfFile, const ProgramHeader& programHeader,
                                  ELFHeader* elfHeader, PagingManager* pagingManager);
};