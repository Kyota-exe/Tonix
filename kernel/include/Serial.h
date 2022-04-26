#pragma once

#include <stdint.h>
#include <stdarg.h>
#include "String.h"

class Serial
{
public:
    static void Printf(const char* string, ...);
    static void Print(const char* string) { Printf(string); }
};