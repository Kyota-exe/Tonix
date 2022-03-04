#pragma once

#include "StringUtilities.h"

[[noreturn]] void Panic(const char* message);
[[noreturn]] void Panic(const char* message, const char* file, unsigned int line, const char* function);

template <typename T>
[[noreturn]] void Panic(const char* message, T value)
{
    Panic(StringUtils::Format(message, (uint64_t)value));
}
