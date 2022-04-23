#pragma once

#include <stdint.h>
#include "Assert.h"

class String
{
private:
    char* buffer;
    uint64_t length;

public:
    char operator[](uint64_t index) const;
    void Push(char c);
    bool Match(uint64_t index, char c, bool assertValidIndex = false) const;
    bool IsNumeric(uint64_t index, bool assertValidIndex = false) const;
    String& operator=(const String& newString);
    uint64_t GetLength() const;
    String Split(char splitCharacter, unsigned int substringIndex) const;
    uint64_t Count(char character) const;
    bool Equals(const String& other) const;
    bool Equals(const char* other) const;
    bool IsEmpty() const;
    String Substring(uint64_t index, uint64_t substringLength) const;
    const char* ToCString() const;
    unsigned int ToUnsignedInt();

    const char* begin() const;
    const char* end() const;

    explicit String(const char* original);
    String(const char* original, uint64_t stringLength);
    String(const String& original);
    String(char c);
    String();
    ~String();

    static String ToString(unsigned long n, int base);
};
