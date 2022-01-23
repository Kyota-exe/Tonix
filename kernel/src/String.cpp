#include "String.h"
#include "Memory/Memory.h"

String& String::operator=(const String& newString)
{
    if (&newString != this)
    {
        delete[] buffer;
        length = newString.length;
        buffer = new char[length + 1];
        MemCopy(buffer, newString.buffer, length + 1);
    }
    return *this;
}

// Converts from null-terminated char*
String::String(const char*& original)
{
    length = 0;

    const char* originalPtr = original;
    while (*originalPtr++ != 0) length++;

    buffer = new char[length + 1];
    MemCopy(buffer, original, length + 1);
}

// Converts from char* with specified length
String::String(const char*& original, uint64_t stringLength)
{
    length = stringLength;

    buffer = new char[length + 1];
    MemCopy(buffer, original, length);
    buffer[length] = 0;
}

uint64_t String::GetLength() const
{
    return length;
}