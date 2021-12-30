#ifndef MISKOS_STRINGUTILITIES_H
#define MISKOS_STRINGUTILITIES_H

#include <stdint.h>
#include <stddef.h>

const char* ToString(int64_t n);
const char* ToHexString(uint64_t n);
size_t StringLength(const char* string);
const char* FormatString(const char* string, int64_t value);

#endif
