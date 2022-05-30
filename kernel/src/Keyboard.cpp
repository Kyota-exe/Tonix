#include "Keyboard.h"
#include "TerminalDevice.h"

bool leftShifting = false;
bool rightShifting = false;
const char chars[] = "\e\e1234567890-=\b\tqwertyuiop[]\n\easdfghjkl;\'`\e\\zxcvbnm,./\e*\e ";
const char shiftChars[] = "\e\e!@#$%^&*()_+\b\tQWERTYUIOP{}\n\eASDFGHJKL:\"~\e|ZXCVBNM<>?\e*\e ";

void Keyboard::SendKeyToTerminal(uint8_t scanCode)
{
    if (scanCode == 0x2a) leftShifting = true;
    else if (scanCode == 0x36) rightShifting = true;
    else if (scanCode == 0xaa) leftShifting = false;
    else if (scanCode == 0xb6) rightShifting = false;

    if (scanCode >= sizeof(chars)) return;
    char c = leftShifting || rightShifting ? shiftChars[scanCode] : chars[scanCode];
    if (c == '\e') return;

    TerminalDevice::instance->KeyboardInput(c);
}