#ifndef MISKOS_STRINGUTILITIES_H
#define MISKOS_STRINGUTILITIES_H

#include <stdint.h>

namespace StringUtils
{
    uint64_t Length(const char* string, uint64_t max = 0);
    const char* Format(const char* string, int64_t value);
    bool Equals(const char* s1, const char* s2);
    uint64_t Count(const char* string, char character);
    uint64_t ToUInt(const char* string);
    uint64_t OctalToUInt(const char* string);
    char* Split(char* string, char splitCharacter, char** stringPtr);
}

#endif
