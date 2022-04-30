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

    if (readCount < currentBufferLength)
    {
        for (uint64_t i = 0; i < currentBufferLength - readCount; ++i)
        {
            inputBuffer[i] = inputBuffer[i + readCount];
        }
    }

    currentBufferLength -= readCount;

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

    pendingWrap = false;
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
    if (!pendingWrap)
        textRenderer->Paint(cursorX, cursorY, backgroundColour);
    else
        textRenderer->Paint(0, cursorY + 1, backgroundColour);

    uint64_t currentIndex = 0;
    while (currentIndex < string.GetLength())
    {
        char c = string[currentIndex++];

        bool eraseCharacter = false;

        switch (c)
        {
            case '\n': // Line Feed
            case '\r': // Carriage Return
            {
                cursorX = 0;
                cursorY++;
                break;
            }
            case '\b': // Backspace
            {
                cursorX--;
                eraseCharacter = true;
                break;
            }
            case '\t': // Horizontal Tab
            {
                cursorX += 4;
                break;
            }
            case '\a': // Terminal Bell
            {
                Panic();
            }
            case '\033': // Escape
            {
                EscapeSequence escapeSequence;

                escapeSequence.controlSequence = string.Match(currentIndex, '[');
                if (escapeSequence.controlSequence) currentIndex++;

                escapeSequence.decPrivate = string.Match(currentIndex, '?');
                if (escapeSequence.decPrivate) currentIndex++;

                if (escapeSequence.controlSequence && string.IsNumeric(currentIndex))
                {
                    while (true)
                    {
                        String numberString;
                        while (string.IsNumeric(currentIndex))
                        {
                            numberString.Push(string[currentIndex++]);
                        }

                        Assert(numberString.GetLength() > 0);
                        escapeSequence.controlArguments.Push(numberString.ToUnsignedInt());

                        if (string.Match(currentIndex, ';')) currentIndex++;
                        else break;
                    }
                }

                Assert(currentIndex < string.GetLength());
                escapeSequence.command = string[currentIndex++];

                ProcessEscapeSequence(escapeSequence);
                break;
            }
            default:
            {
                if (pendingWrap)
                {
                    cursorX = 0;
                    cursorY++;
                    pendingWrap = false;
                }

                textRenderer->Print(c, cursorX, cursorY, textColour, textBgColour);
                cursorX++;

                if (CursorAtRightEdge()) pendingWrap = true;
            }
        }

        if (CursorAtRightEdge())
        {
            cursorX = textRenderer->CharsPerLine() - 1;
        }

        if (cursorX < 0) cursorX = 0;
        if (cursorY < 0) cursorY = 0;
        Assert(cursorY < textRenderer->CharsPerColumn());

        if (eraseCharacter)
        {
            textRenderer->Paint(cursorX, cursorY, backgroundColour);
        }
    }

    // Render cursor
    if (!pendingWrap)
        textRenderer->Print('_', cursorX, cursorY, textColour, backgroundColour);
    else
        textRenderer->Print('_', 0, cursorY + 1, textColour, backgroundColour);
}

void Terminal::ProcessEscapeSequence(const EscapeSequence& escapeSequence)
{
    if (escapeSequence.controlSequence)
    {
        Assert(!escapeSequence.rightParentheses);
        ProcessControlSequence(escapeSequence.command, escapeSequence.controlArguments, escapeSequence.decPrivate);
    }
    else
    {
        Assert(!escapeSequence.decPrivate);

        Assert(escapeSequence.command == 'M');
        cursorY--;
    }
}

void Terminal::ProcessControlSequence(char command, const Vector<unsigned int>& arguments, bool decPrivate)
{
    uint64_t argCount = arguments.GetLength();

    switch (command)
    {
        case 'H':
            if (argCount == 0) { cursorX = cursorY = 0; break; }
            Assert(argCount == 2);
            cursorY = arguments.Get(0);
            cursorX = arguments.Get(1);
            break;
        case 'f':
            Assert(argCount == 2);
            cursorY = arguments.Get(0);
            cursorX = arguments.Get(1);
            break;
        case 'A':
            Assert(argCount == 1);
            cursorY -= arguments.Get(0);
            break;
        case 'B':
            Assert(argCount == 1);
            cursorY += arguments.Get(0);
            break;
        case 'C':
            Assert(argCount == 1);
            cursorX += arguments.Get(0);
            break;
        case 'D':
            Assert(argCount == 1);
            cursorX -= arguments.Get(0);
            break;
        case 'E':
            Assert(argCount == 1);
            cursorY += arguments.Get(0);
            cursorX = 0;
            break;
        case 'F':
            Assert(argCount == 1);
            cursorY -= arguments.Get(0);
            cursorX = 0;
            break;
        case 'G':
            Assert(argCount == 1);
            cursorX = arguments.Get(0);
            break;
        case 'J':
            if (argCount == 0) EraseScreenFrom(cursorX, cursorY);
            else
            {
                Assert(argCount == 1);
                switch (arguments.Get(0))
                {
                    case 0:
                        EraseScreenFrom(cursorX, cursorY);
                        break;
                    case 1:
                        EraseRangeInclusive(0, 0, cursorX, cursorY);
                        break;
                    case 2:
                        EraseScreenFrom(0, 0);
                        break;
                    default:
                        Panic();
                }
            }
            break;
        case 'K':
            if (argCount == 0) EraseRangeInclusive(cursorX, cursorY, textRenderer->CharsPerLine() - 1, cursorY);
            else
            {
                Assert(argCount == 1);
                switch (arguments.Get(0))
                {
                    case 0:
                        EraseRangeInclusive(cursorX, cursorY, textRenderer->CharsPerLine() - 1, cursorY);
                        break;
                    case 1:
                        EraseRangeInclusive(0, cursorY, cursorX, cursorY);
                        break;
                    case 2:
                        EraseRangeInclusive(0, cursorY, textRenderer->CharsPerLine() - 1, cursorY);
                        break;
                }
            }
            break;
        case 'm':
            if (argCount == 0) ResetColors();
            for (unsigned int arg : arguments)
            {
                switch (arg)
                {
                    case 0:
                        ResetColors();
                        break;
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

void Terminal::ResetColors()
{
    textColour = originalTextColour;
    textBgColour = originalTextBgColour;
}

void Terminal::EraseRangeInclusive(long minX, long minY, long maxX, long maxY)
{
    for (long y = minY; y <= maxY; ++y)
    {
        for (long x = (y == minY) ? minX : 0; x <= maxX; ++x)
        {
            textRenderer->Paint(x, y, backgroundColour);
        }
    }
}

void Terminal::EraseScreenFrom(long x, long y)
{
    EraseRangeInclusive(x, y, textRenderer->CharsPerLine() - 1, textRenderer->CharsPerColumn() - 1);
}

bool Terminal::CursorAtRightEdge()
{
    return cursorX >= textRenderer->CharsPerLine();
}