#pragma once

#include <stdint.h>
#include <stdarg.h>
#include "String.h"

class Serial
{
public:
    static void Log(const char* string, ...);
};