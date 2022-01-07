#ifndef MISKOS_PIT_H
#define MISKOS_PIT_H

#include <stdint.h>
#include "IO.h"

uint16_t PITGetTick();
void PITSetFrequency(uint64_t frequency);
void PITSetReloadValue(uint16_t reloadValue);

extern const uint64_t PIT_BASE_FREQUENCY;

#endif
