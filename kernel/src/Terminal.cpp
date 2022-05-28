#include "Terminal.h"
#include "Memory/Memory.h"
#include "Serial.h"
#include "Vector.h"

void Terminal::Write(const String& string)
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
                if (cursorY >= textRenderer->GetWindowSize().rowCount)
                {
                    Assert(cursorY == textRenderer->GetWindowSize().rowCount);
                    textRenderer->ScrollDown(backgroundColour);
                    cursorY--;
                }
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
                Panic();
                cursorX += 4;
                break;
            }
            case '\017':
            {
                Warn("SI is not implemented");
                break;
            }
            case '\a': // Terminal Bell
            {
                Warn("BEL is not implemented");
                break;
            }
            case '\033': // Escape
            {
                EscapeSequence escapeSequence;

                escapeSequence.controlSequence = string.Match(currentIndex, '[');
                if (escapeSequence.controlSequence) currentIndex++;

                escapeSequence.decPrivate = string.Match(currentIndex, '?');
                if (escapeSequence.decPrivate) currentIndex++;

                escapeSequence.rightParentheses = string.Match(currentIndex, ')');
                if (escapeSequence.rightParentheses) currentIndex++;

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
            cursorX = textRenderer->GetWindowSize().columnCount - 1;
        }

        if (cursorX < textRenderer->GetWindowSize().columnCount - 1)
        {
            pendingWrap = false;
        }

        if (cursorX < 0) cursorX = 0;
        if (cursorY < 0) cursorY = 0;
        Assert(cursorY < textRenderer->GetWindowSize().rowCount);

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

Terminal::Terminal() : textRenderer(new TextRenderer()),
                       textColour(0xd7, 0xe3, 0xfe),
                       backgroundColour(0x26, 0x2c, 0x37),
                       originalTextColour(textColour),
                       textBgColour(backgroundColour)
{
    EraseScreenFrom(0, 0);
}

Terminal::~Terminal()
{
    delete textRenderer;
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

        if (escapeSequence.rightParentheses)
        {
            Assert(escapeSequence.command == '0');
            Warn("ESC)0 does nothing; special character sets are not implemented");
        }
        else
        {
            Assert(escapeSequence.command == 'M');
            cursorY--;
        }
    }
}

void Terminal::ProcessControlSequence(char command, const Vector<int>& arguments, bool decPrivate)
{
    uint64_t argCount = arguments.GetLength();

    if (decPrivate)
    {
        Assert(command == 'h');
        Assert(argCount == 1);
        Assert(arguments.Get(0) == 7);
        Warn("ESC[?7h does nothing; wraparound mode is always enabled");
        return;
    }

    switch (command)
    {
        case 'H':
        case 'f':
            if (argCount == 0) { cursorX = cursorY = 0; break; }
            Assert(argCount == 2);
            cursorY = arguments.Get(0) - 1;
            cursorX = arguments.Get(1) - 1;
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
            cursorX = arguments.Get(0) - 1;
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
            if (argCount == 0) EraseRangeInclusive(cursorX, cursorY, textRenderer->GetWindowSize().columnCount - 1, cursorY);
            else
            {
                Assert(argCount == 1);
                switch (arguments.Get(0))
                {
                    case 0:
                        EraseRangeInclusive(cursorX, cursorY, textRenderer->GetWindowSize().columnCount - 1, cursorY);
                        break;
                    case 1:
                        EraseRangeInclusive(0, cursorY, cursorX, cursorY);
                        break;
                    case 2:
                        EraseRangeInclusive(0, cursorY, textRenderer->GetWindowSize().columnCount - 1, cursorY);
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
                        textBgColour = backgroundColour;
                        break;
                    case 1:
                        Warn("Bold text is not implemented");
                        break;
                    default:
                        Panic();
                }
            }
            break;
        case 'r':
            Assert(argCount == 2);
            Assert(arguments.Get(0) == 1);
            Assert(arguments.Get(1) == textRenderer->GetWindowSize().rowCount);
            break;
        case 'l':
            Assert(argCount == 1);
            Assert(arguments.Get(0) == 4);
            Warn("ESC[4l does nothing; jump scrolling is always enabled");
            break;
        case 'd':
            Assert(argCount == 1);
            cursorY = arguments.Get(0) - 1;
            break;
        default:
            Panic();
    }
}

void Terminal::ResetColors()
{
    textColour = originalTextColour;
    textBgColour = backgroundColour;
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
    EraseRangeInclusive(x, y, textRenderer->GetWindowSize().columnCount - 1, textRenderer->GetWindowSize().rowCount - 1);
}

bool Terminal::CursorAtRightEdge()
{
    return cursorX >= textRenderer->GetWindowSize().columnCount;
}

WindowSize Terminal::GetWindowSize()
{
    return textRenderer->GetWindowSize();
}
