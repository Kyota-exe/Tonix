#pragma once

#include <stdint.h>
#include "String.h"
#include "StringUtilities.h"

class Serial
{
public:
    static void Print(const char* string, const char* end = "\n");
    static void Print(const String& string, const char* end = "\n");

    template <typename T>
    static void Printf(const char* string, T value, const char* end = "\n")
    {
        char* formattedString = new char[128];
        StringUtils::Format(formattedString, string, (uint64_t)value);
        Print(formattedString, end);
        delete[] formattedString;
    }
};