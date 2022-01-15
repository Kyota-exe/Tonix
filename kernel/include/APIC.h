#ifndef MISKOS_APIC_H
#define MISKOS_APIC_H

#include <stdint.h>

void ActivateLAPIC();
void SetLAPICTimerMode(uint8_t mode);
void SetLAPICTimerFrequency(uint64_t frequency);
void SetLAPICTimerMask(bool mask);
void LAPICSendEOI();
void CalibrateLAPICTimer();

#endif
