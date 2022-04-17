#include "Keyboard.h"
#include "Terminal.h"

bool leftShifting = false;
bool rightShifting = false;
char chars[] = "\e\e1234567890-=\b\eqwertyuiop[]\n\easdfghjkl;\'`\e\\zxcvbnm,./\e*\e ";
char shiftChars[] = "\e\e!@#$%^&*()_+\b\eQWERTYUIOP{}\n\eASDFGHJKL:\"~\e|ZXCVBNM<>?\e*\e ";

void Keyboard::SendKeyToTerminal(uint8_t scanCode)
{
    if (scanCode == 0x2a) leftShifting = true;
    else if (scanCode == 0x36) rightShifting = true;
    else if (scanCode == 0xaa) leftShifting = false;
    else if (scanCode == 0xb6) rightShifting = false;

    if (scanCode >= sizeof(chars)) return;
    char c = leftShifting || rightShifting ? shiftChars[scanCode] : chars[scanCode];
    if (c == '\e') return;

    Terminal::instance->InputCharacter(c);
}