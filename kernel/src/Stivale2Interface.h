#ifndef MISKOS_STIVALE2INTERFACE_H
#define MISKOS_STIVALE2INTERFACE_H

#include <stdint.h>
#include "stivale2.h"

stivale2_struct_tag_terminal* GetStivale2Terminal(struct stivale2_struct *stivale2Struct);
void *GetStivale2Tag(struct stivale2_struct *stivale2Struct, uint64_t id);

#endif
