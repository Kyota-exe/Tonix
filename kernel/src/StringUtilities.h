#ifndef MISKOS_STRINGUTILITIES_H
#define MISKOS_STRINGUTILITIES_H

#include <stdint.h>
#include <stddef.h>

char* ToString(int64_t n);
char* ToHexString(uint64_t n);
size_t StringLength(const char* string);

#endif
