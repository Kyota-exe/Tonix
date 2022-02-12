#pragma once

#include <stdint.h>
#include "Process.h"

class PagingManager;
struct ELFHeader;
struct ProgramHeader;

class ELFLoader
{
public:
    static void LoadELF(const String& path, Process* process);
private:
    static void LoadProgramHeader(int elfFile, const ProgramHeader& programHeader,
                                  ELFHeader* elfHeader, PagingManager* pagingManager);
};