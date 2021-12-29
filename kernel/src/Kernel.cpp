#include "Stivale2Interface.h"
#include "SerialOutput.h"

extern "C" void _start(struct stivale2_struct *stivale2Struct)
{
    InitializeStivale2Interface(stivale2Struct);
    SerialOutput serial = SerialOutput(0xe9);

    serial.Print("Kernel ELF successfully loaded");

    while (true) asm("hlt");
}
