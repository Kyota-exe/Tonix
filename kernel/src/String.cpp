#include "String.h"
#include "Memory/Memory.h"
#include "Serial.h"
#include "Assert.h"

String String::Split(char splitCharacter, unsigned int substringIndex) const
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

    Panic();
}

uint64_t String::Count(char character) const
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

bool String::Equals(const String& other) const
{
    return Equals(other.buffer);
}

bool String::Equals(const char* other) const
{
    const char* bufferPtr = buffer;

    while (*bufferPtr != 0 && *other != 0)
    {
        if (*bufferPtr++ != *other++) return false;
    }

    return *bufferPtr == 0 && *other == 0;
}

char String::operator[](uint64_t index) const
{
    Assert(index < length);
    return buffer[index];
}

void String::Push(char c)
{
    Assert(c != 0);

    length++;

    char* newBuffer = new char[length + 1];
    MemCopy(newBuffer, buffer, length - 1);
    newBuffer[length - 1] = c;
    newBuffer[length] = 0;

    delete[] buffer;
    buffer = newBuffer;
}

bool String::Match(uint64_t index, char c, bool assertValidIndex) const
{
    if (assertValidIndex && index >= length) return false;

    Assert(index < length);
    return buffer[index] == c;
}

bool String::IsNumeric(uint64_t index, bool assertValidIndex) const
{
    if (assertValidIndex && index >= length) return false;

    Assert(index < length);
    return buffer[index] >= '0' && buffer[index] <= '9';
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

    for (uint64_t i = 0; i < length; ++i) Assert(buffer[i] != 0);
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

String::String(char c)
{
    Assert(c != 0);

    length = 1;
    buffer = new char[2];
    buffer[0] = c;
    buffer[1] = 0;
}

String::~String()
{
    delete[] buffer;
}

uint64_t String::GetLength() const
{
    return length;
}

bool String::IsEmpty() const
{
    return Equals("");
}

String String::Substring(uint64_t index, uint64_t substringLength) const
{
    Assert(index + substringLength <= GetLength());

    char newBuffer[substringLength + 1];
    MemCopy(newBuffer, buffer + index, substringLength);
    newBuffer[substringLength] = 0;

    return String(newBuffer);
}

unsigned int String::ToUnsignedInt()
{
    unsigned int number = 0;

    for (uint64_t i = 0; i < length; ++i)
    {
        Assert(IsNumeric(i, true));
        number *= 10;
        number += buffer[i] - '0';
    }

    return number;
}

const char* String::ToRawString() const
{
    return buffer;
}

const char* String::begin() const
{
    return buffer;
}

const char* String::end() const
{
    return buffer + GetLength();
}