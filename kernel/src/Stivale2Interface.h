#ifndef MISKOS_STIVALE2INTERFACE_H
#define MISKOS_STIVALE2INTERFACE_H

#include <stdint.h>
#include <stddef.h>
#include "stivale2.h"
#include "StringUtilities.h"

void InitializeStivale2Interface(stivale2_struct *stivale2Struct);
void Stivale2TerminalWrite(const char* string, const char* end = "\n");
stivale2_struct_tag_memmap* GetMemoryMap(stivale2_struct *stivale2Struct);

#endif
