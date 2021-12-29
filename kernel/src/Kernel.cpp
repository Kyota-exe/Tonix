#include "Stivale2Interface.h"
#include "SerialOutput.h"

extern "C" void _start(struct stivale2_struct *stivale2Struct)
{
    InitializeStivale2Interface(stivale2Struct);
    SerialOutput serial = SerialOutput(0xe9);

    serial.Print("Kernel ELF successfully loaded");

    serial.Print(FormatString("Hello %x yes", 0x346a2));

    while (true) asm("hlt");
}
