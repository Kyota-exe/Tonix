#pragma once

#include <stdint.h>

class Keyboard
{
public:
    static void SendKeyToTerminal(uint8_t scanCode);
};