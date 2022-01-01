#ifndef MISKOS_QEMUSERIAL_H
#define MISKOS_QEMUSERIAL_H

#include "IO.h"
#include "StringUtilities.h"

namespace Serial
{
    void Print(const char* string, const char* end = "\n");
    void Printf(const char* string, int64_t value, const char* end = "\n");
}

#endif
