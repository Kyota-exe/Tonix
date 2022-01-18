#ifndef MISKOS_PANIC_H
#define MISKOS_PANIC_H

#include "StringUtilities.h"

void Panic(const char* message);
void KAssert(bool expression, const char* message);

template <typename T>
void KAssert(bool expression, const char* message, T value)
{
    KAssert(expression, String::Format(message, (uint64_t)value));
}

template <typename T>
void Panic(const char* message, T value)
{
    Panic(String::Format(message, (uint64_t)value));
}

#endif
