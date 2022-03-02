#include "Terminal.h"
#include "Serial.h"

const char* FONT_PATH = "/fonts/Uni2-Terminus20x10.psf";
constexpr int TEXT_CHARACTER_SPACING = 0;

uint64_t Terminal::Read(void* buffer, uint64_t count)
{
    (void)buffer;
    (void)count;
    return 0;
}

uint64_t Terminal::Write(const void* buffer, uint64_t count)
{
    textRenderer->Print(String((const char*)buffer));
    return count;
}

Terminal::Terminal(const String& name, uint32_t inodeNum) :
    Device(name, inodeNum),
    textRenderer(new TextRenderer(String(FONT_PATH),Colour(255, 255, 255),
                                  TEXT_CHARACTER_SPACING,Colour(0, 0, 0))),
    textColour(Colour(255, 255, 255)) { }

Terminal::~Terminal()
{
    delete textRenderer;
}