#pragma once

#include <stdint.h>
#include "stivale2.h"

void InitializeStivale2Interface(stivale2_struct *stivale2Struct);
void* GetStivale2Tag(uint64_t id);
