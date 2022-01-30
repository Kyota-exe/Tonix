#include "Terminal.h"
#include "Serial.h"

uint64_t Terminal::Read(void* buffer, uint64_t count)
{
    (void)buffer;
    (void)count;
    return 0;
}

uint64_t Terminal::Write(const void* buffer, uint64_t count)
{
    String string = String((const char*)buffer, count);

    Serial::Print(string);

    return count;
}

Terminal::Terminal(const String& name, uint32_t inodeNum) : Device(new String(name), inodeNum) { }

Terminal::~Terminal()
{
    delete name;
}