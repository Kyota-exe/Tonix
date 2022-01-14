#ifndef MISKOS_APIC_H
#define MISKOS_APIC_H

#include <stdint.h>

void ActivateLAPIC();
void LAPICSendEOI();
uint64_t CalibrateLAPICTimer();

#endif
