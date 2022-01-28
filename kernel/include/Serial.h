#ifndef MISKOS_QEMUSERIAL_H
#define MISKOS_QEMUSERIAL_H

#include <stdint.h>
#include "String.h"
#include "StringUtilities.h"

namespace Serial
{
    void Print(const char* string, const char* end = "\n");
    void Print(const String& string, const char* end = "\n");

    template <typename T>
    void Printf(const char* string, T value, const char* end = "\n")
    {
        Print(StringUtils::Format(string, (int64_t)value), end);
    }
}

#endif
