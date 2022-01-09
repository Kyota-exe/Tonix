#ifndef MISKOS_STRINGUTILITIES_H
#define MISKOS_STRINGUTILITIES_H

#include <stdint.h>

uint64_t StringLength(const char* string, uint64_t max = 0);
const char* FormatString(const char* string, int64_t value);
bool StringEquals(const char* s1, const char* s2);
uint64_t StringToUInt(const char* string);
uint64_t StringOctalToUInt(const char* string);

#endif
