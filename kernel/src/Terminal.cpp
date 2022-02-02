#include "Terminal.h"
#include "Serial.h"

const char* fontPath = "/fonts/MicroKnight_v1.0.psf";

uint64_t Terminal::Read(void* buffer, uint64_t count)
{
    (void)buffer;
    (void)count;
    return 0;
}

uint64_t Terminal::Write(const void* buffer, uint64_t count)
{
    textRenderer->Print(String((const char*)buffer), cursorX, cursorY, Colour(255, 255, 255));
    return count;
}

Terminal::Terminal(const String& name, uint32_t inodeNum) : Device(new String(name), inodeNum)
{
    textRenderer = new TextRenderer(String(fontPath));
}

Terminal::~Terminal()
{
    delete name;
    delete textRenderer;
}