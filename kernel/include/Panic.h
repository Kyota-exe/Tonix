#pragma once

#include "StringUtilities.h"

void Panic(const char* message);
void Panic(const char* message, const char* file, unsigned int line, const char* function);

template <typename T>
void Panic(const char* message, T value)
{
    Panic(StringUtils::Format(message, (uint64_t)value));
}
