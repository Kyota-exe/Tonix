#include "String.h"
#include "Memory/Memory.h"
#include "Panic.h"

String String::Split(char splitCharacter, unsigned int substringIndex)
{
    unsigned int currentSubstring = 0;
    uint64_t substringBegin = 0;
    uint64_t substringLength = 0;

    for (uint64_t i = 0; i <= GetLength(); ++i)
    {
        if (buffer[i] == splitCharacter || buffer[i] == 0)
        {
            if (currentSubstring == substringIndex)
            {
                return Substring(substringBegin, substringLength);
            }
            currentSubstring++;
            substringLength = 0;
            substringBegin = i + 1;
            continue;
        }
        substringLength++;
    }

    Panic("Could not find substring from splitting string.");
    return {};
}

uint64_t String::Count(char character)
{
    uint64_t count = 0;

    char* ptr = buffer;
    while (*ptr != 0)
    {
        if (*ptr == character) count++;
        ptr++;
    }

    return count;
}

bool String::Equals(const String& other)
{
    const char* ptr1 = buffer;
    const char* ptr2 = other.buffer;

    while (*ptr1 != 0 && *ptr2 != 0)
    {
        if (*ptr1++ != *ptr2++) return false;
    }

    return *ptr1 == 0 && *ptr2 == 0;
}

char& String::operator[](uint64_t index)
{
    return buffer[index];
}

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
String::String(const char* original)
{
    length = 0;

    const char* originalPtr = original;
    while (*originalPtr++ != 0) length++;

    buffer = new char[length + 1];
    MemCopy(buffer, original, length + 1);
}

// Converts from char* with specified length
String::String(const char* original, uint64_t stringLength)
{
    length = stringLength;

    buffer = new char[length + 1];
    MemCopy(buffer, original, length);
    buffer[length] = 0;
}

String::String(const String& original)
{
    length = original.length;
    buffer = new char[length + 1];
    MemCopy(buffer, original.buffer, length + 1);
}

String::String()
{
    length = 0;
    buffer = nullptr;
}

String::~String()
{
    delete[] buffer;
}

uint64_t String::GetLength() const
{
    return length;
}

bool String::IsEmpty()
{
    return this->Equals(String(""));
}

String String::Substring(uint64_t index, uint64_t substringLength)
{
    KAssert(index + substringLength <= GetLength(), "Substring exceeds bounds of substring.");

    char newBuffer[substringLength + 1];
    MemCopy(newBuffer, buffer + index, substringLength);
    newBuffer[substringLength] = 0;

    return String(newBuffer);
}