#ifndef MISKOS_QEMUSERIAL_H
#define MISKOS_QEMUSERIAL_H

#include "IO.h"
#include "StringUtilities.h"

class Serial
{
public:
    static void Print(const char* string, const char* end = "\n");
    static void Printf(const char* string, int64_t value, const char* end = "\n");

private:
    static const uint16_t PORT = 0xe9;
    Serial() { }
};

#endif
