#pragma once
#include "Panic.h"
#include "String.h"

#define Assert(assertion) if (!(assertion)) Panic(#assertion, __FILE__, __LINE__, __func__)