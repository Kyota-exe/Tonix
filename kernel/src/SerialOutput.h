#ifndef MISKOS_QEMUSERIAL_H
#define MISKOS_QEMUSERIAL_H

#include "IO.h"
#include "StringUtilities.h"

class SerialOutput
{
public:
    uint16_t port;

    void Print(const char* string, const char* end = "\n");
    SerialOutput(uint16_t _port);
};

#endif
