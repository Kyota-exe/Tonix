#include "Terminal.h"
#include "Memory/Memory.h"
#include "Serial.h"
#include "Vector.h"
#include "Scheduler.h"

const char* FONT_PATH = "/fonts/Uni2-Terminus20x10.psf";
constexpr int CHARACTER_SPACING = 0;
constexpr uint64_t BUFFER_SIZE = 2048;

Terminal* Terminal::instance = nullptr;

uint64_t Terminal::Read(void* buffer, uint64_t count)
{
    if (count > currentBufferLength)
    {
        Scheduler* scheduler = Scheduler::GetScheduler();
        unblockQueue.Push(scheduler->currentTask.pid);
        scheduler->SuspendSystemCall();
    }

    uint64_t readCount = count > currentBufferLength ? currentBufferLength : count;
    MemCopy(buffer, inputBuffer, readCount);
    currentBufferLength -= readCount;

    if (count < currentBufferLength)
    {
        for (uint64_t i = 0; i < currentBufferLength; ++i)
        {
            inputBuffer[i] = inputBuffer[i + readCount];
        }
    }

    return readCount;
}

uint64_t Terminal::Write(const void* buffer, uint64_t count)
{
    textRenderer->Print(String(reinterpret_cast<const char*>(buffer), count));
    return count;
}

void Terminal::InputCharacter(char c)
{
    Assert(currentBufferLength < BUFFER_SIZE);
    inputBuffer[currentBufferLength++] = c;

    Serial::Print(String(c));
    textRenderer->Print(String(c));

    if (c == '\n' && unblockQueue.GetLength() > 0)
    {
        Scheduler::Unblock(unblockQueue.Pop());
    }
}

Terminal::Terminal(const String& name, uint32_t inodeNum) : Device(name, inodeNum)
{
    Assert(instance == nullptr);
    instance = this;

    inputBuffer = new char[BUFFER_SIZE];
    currentBufferLength = 0;

    auto textColour = Colour(255, 255, 255);
    auto backgroundColour = Colour(0, 0, 0);
    textRenderer = new TextRenderer(String(FONT_PATH), textColour, CHARACTER_SPACING, backgroundColour);
}

Terminal::~Terminal()
{
    delete inputBuffer;
    delete textRenderer;
    instance = nullptr;
}