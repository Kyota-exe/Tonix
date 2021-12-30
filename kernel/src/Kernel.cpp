#include "Stivale2Interface.h"
#include "IDT.h"
#include "Serial.h"

extern "C" void _start(struct stivale2_struct *stivale2Struct)
{
    InitializeStivale2Interface(stivale2Struct);

    Serial::Print("Kernel ELF successfully loaded\n");

    LoadIDT();

    while (true) asm ("hlt");
}
