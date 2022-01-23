#ifndef MISKOS_STRING_H
#define MISKOS_STRING_H

#include <stdint.h>

class String
{
private:
    char* buffer;
    uint64_t length;

public:
    String& operator=(const String& newString);
    uint64_t GetLength() const;

    explicit String(const char*& original);
    String(const char*& original, uint64_t stringLength);
};

#endif
