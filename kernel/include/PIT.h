#pragma once

#include <stdint.h>

uint16_t PITGetTick();
void PITSetReloadValue(uint16_t reloadValue);
void PITSetFrequency(uint64_t frequency);

extern const uint64_t PIT_BASE_FREQUENCY;
