#pragma once

#include <stdint.h>
#include "Task.h"

void* FileMap(Task* task, void* addr, uint64_t length, int protection, int flags, int descriptor, int offset);