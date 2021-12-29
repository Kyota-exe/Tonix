#include <stddef.h>
#include "Stivale2Interface.h"

extern "C" void _start(struct stivale2_struct *stivale2Struct)
{
    struct stivale2_struct_tag_terminal* terminalTag = GetStivale2Terminal(stivale2Struct);
    if (terminalTag == NULL) while (true) asm("hlt");

    void (*terminalWrite)(const char* string, size_t size) = (void(*)(const char*, size_t))terminalTag->term_write;

    terminalWrite("Kernel successfully loaded", 26);

    while (true) asm("hlt");
}
