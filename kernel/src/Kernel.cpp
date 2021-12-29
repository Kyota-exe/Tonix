#include "Stivale2Interface.h"
#include "StringUtilities.h"

extern "C" void _start(struct stivale2_struct *stivale2Struct)
{
    InitializeStivale2Interface(stivale2Struct);
    Stivale2TerminalWrite("Kernel successfully loaded");
    Stivale2TerminalWrite(StringLength("hello"));

    while (true) asm("hlt");
}
