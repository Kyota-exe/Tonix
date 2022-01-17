#ifndef MISKOS_QEMUSERIAL_H
#define MISKOS_QEMUSERIAL_H

#include <stdint.h>
#include "StringUtilities.h"

namespace Serial
{
    void Print(const char* string, const char* end = "\n");

    template <typename T>
    void Printf(const char* string, T value, const char* end = "\n")
    {
        Print(String::Format(string, (uint64_t)value), end);
    }
}

#endif
