#pragma once

#include <stdint.h>

void* FileMap(void* addr, uint64_t length, int protection, int flags, int descriptor, int offset);