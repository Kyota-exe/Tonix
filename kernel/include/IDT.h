#pragma once

#include <stdint.h>

class IDT
{
public:
    static void Initialize();
    static void Load();
private:
    static void SetInterruptHandler(int interrupt, uint64_t handler, uint8_t ring = 0, uint8_t ist = 0);
    static void InitializeInterruptHandlers();
};