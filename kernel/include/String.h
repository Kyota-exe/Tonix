#ifndef MISKOS_STRING_H
#define MISKOS_STRING_H

#include <stdint.h>

class String
{
private:
    char* buffer;
    uint64_t length;

public:
    char& operator[](uint64_t index);
    String& operator=(const String& newString);
    uint64_t GetLength() const;
    String Split(char splitCharacter, unsigned int substringIndex);
    uint64_t Count(char character);
    bool Equals(const String& other);
    bool IsEmpty();
    String Substring(uint64_t index, uint64_t substringLength);
    const char* ToCString() const;

    //temp
    const char* begin() {return buffer;}

    explicit String(const char* original);
    String(const char* original, uint64_t stringLength);
    String(const String& original);
    String();
    ~String();
};

#endif
