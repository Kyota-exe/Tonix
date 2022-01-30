#pragma once

#include <stdint.h>
#include "stivale2.h"

void InitializeStivale2Interface(stivale2_struct *stivale2Struct);
void Stivale2TerminalWrite(const char* string, const char* end = "\n");
void* GetStivale2Tag(uint64_t id);
