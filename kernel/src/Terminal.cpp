#include "Terminal.h"
#include "Memory/Memory.h"
#include "Serial.h"
#include "Vector.h"
#include "Scheduler.h"

constexpr uint64_t INPUT_BUFFER_SIZE = 2048;

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
    Print(String(reinterpret_cast<const char*>(buffer), count));
    return count;
}

void Terminal::InputCharacter(char c)
{
    if (c == '\b')
    {
        if (currentBufferLength == 0) return;
        currentBufferLength--;
    }
    else
    {
        Assert(currentBufferLength < INPUT_BUFFER_SIZE);
        inputBuffer[currentBufferLength++] = c;
    }

    Print(String(c));

    if (c == '\n' && unblockQueue.GetLength() > 0)
    {
        Scheduler::Unblock(unblockQueue.Pop());
    }
}

Terminal::Terminal(const String& name, uint32_t inodeNum) : Device(name, inodeNum),
                                                            textRenderer(new TextRenderer()),
                                                            backgroundColour(Colour(0, 0, 0)),
                                                            originalTextColour(Colour(0, 0, 0)),
                                                            originalTextBgColour(Colour(0, 0, 0)),
                                                            textColour(Colour(255, 255, 255)),
                                                            textBgColour(Colour(0, 0, 0)),
                                                            cursorX(0), cursorY(0)
{
    originalTextColour = textColour;
    originalTextBgColour = textBgColour;

    Assert(instance == nullptr);
    instance = this;

    inputBuffer = new char[INPUT_BUFFER_SIZE];
    currentBufferLength = 0;
}

Terminal::~Terminal()
{
    delete inputBuffer;
    delete textRenderer;
    instance = nullptr;
}

void Terminal::Print(const String& string)
{
    // Erase previous cursor
    textRenderer->Paint(cursorX, cursorY, backgroundColour);

    uint64_t currentIndex = 0;
    while (currentIndex < string.GetLength())
    {
        char c = string[currentIndex++];

        bool eraseCharacter = false;

        switch (c)
        {
            case '\n': // Line Feed
            {
                nextCursorXPerLine.Push(cursorX);
                cursorX = 0;
                cursorY += textRenderer->FontHeight();
                break;
            }
            case '\b': // Backspace
            {
                cursorX -= textRenderer->FontWidth();
                eraseCharacter = true;
                break;
            }
            case '\t': // Horizontal Tab
            {
                cursorX += textRenderer->FontWidth() * 4;
                break;
            }
            case '\a': // Terminal Bell
            {
                Panic();
            }
            case '\033': // Escape
            {
                bool hasCSI = string.Match(currentIndex, '[');
                if (hasCSI) currentIndex++;

                Vector<unsigned int> arguments;
                if (string.IsNumeric(currentIndex))
                {
                    while (true)
                    {
                        String numberString;
                        while (string.IsNumeric(currentIndex))
                        {
                            numberString.Push(string[currentIndex++]);
                        }

                        Assert(numberString.GetLength() > 0);
                        arguments.Push(numberString.ToUnsignedInt());

                        if (string.Match(currentIndex, ';')) currentIndex++;
                        else break;
                    }
                }

                Assert(currentIndex < string.GetLength());
                char command = string[currentIndex++];

                ProcessEscapeSequence(command, hasCSI, arguments);
                break;
            }
            default:
            {
                textRenderer->Print(c, cursorX, cursorY, textColour, textBgColour);
                cursorX += textRenderer->FontWidth();
            }
        }

        // Right edge cursor wrapping
        if (cursorX + textRenderer->FontWidth() > textRenderer->ScreenWidth())
        {
            nextCursorXPerLine.Push(cursorX - textRenderer->FontWidth());
            cursorX = 0;
            cursorY += textRenderer->FontHeight();
        }
        // Left edge cursor wrapping
        else if (cursorX < 0)
        {
            Assert(nextCursorXPerLine.GetLength() > 0);
            cursorX = nextCursorXPerLine.Pop();
            cursorY -= textRenderer->FontHeight();
        }

        Assert(cursorX >= 0);
        Assert(cursorY >= 0);
        Assert(cursorY < textRenderer->ScreenHeight());

        if (eraseCharacter)
        {
            textRenderer->Paint(cursorX, cursorY, backgroundColour);
        }
    }

    // Render cursor
    textRenderer->Print('_', cursorX, cursorY, textColour, backgroundColour);
}

void Terminal::ProcessEscapeSequence(char command, bool hasCSI, const Vector<unsigned int> &arguments)
{
    uint64_t argCount = arguments.GetLength();

    if (hasCSI)
    {
        switch (command)
        {
            case 'H':
                if (argCount == 0) { cursorX = cursorY = 0; break; }
                Assert(argCount == 2);
                cursorY = arguments.Get(0) * textRenderer->FontHeight();
                cursorX = arguments.Get(1) * textRenderer->FontWidth();
                break;
            case 'f':
                Assert(argCount == 2);
                cursorY = arguments.Get(0) * textRenderer->FontHeight();
                cursorX = arguments.Get(1) * textRenderer->FontWidth();
                break;
            case 'A':
                Assert(argCount == 1);
                cursorY -= arguments.Get(0) * textRenderer->FontHeight();
                break;
            case 'B':
                Assert(argCount == 1);
                cursorY += arguments.Get(0) * textRenderer->FontHeight();
                break;
            case 'C':
                Assert(argCount == 1);
                cursorX += arguments.Get(0) * textRenderer->FontWidth();
                break;
            case 'D':
                Assert(argCount == 1);
                cursorX -= arguments.Get(0) * textRenderer->FontWidth();
                break;
            case 'E':
                Assert(argCount == 1);
                cursorY += arguments.Get(0) * textRenderer->FontHeight();
                cursorX = 0;
                break;
            case 'F':
                Assert(argCount == 1);
                cursorY -= arguments.Get(0) * textRenderer->FontHeight();
                cursorX = 0;
                break;
            case 'G':
                Assert(argCount == 1);
                cursorX = arguments.Get(0) * textRenderer->FontWidth();
                break;
            case 'm':
                for (unsigned int arg : arguments)
                {
                    switch (arg)
                    {
                        case 30 ... 38:
                            textColour = Colour::FromANSICode(arg);
                            break;
                        case 40 ... 48:
                            textBgColour = Colour::FromANSICode(arg);
                            break;
                        case 39:
                            textColour = originalTextColour;
                            break;
                        case 49:
                            textBgColour = originalTextBgColour;
                            break;
                        default:
                            Panic();
                    }
                }
                break;
            default:
                Panic();
        }
    }
    else
    {
        switch (command)
        {
            case 'M':
                cursorY -= textRenderer->FontHeight();
                break;
            default:
                Panic();
        }
    }
}