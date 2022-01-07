#ifndef MISKOS_APIC_H
#define MISKOS_APIC_H

#include <stdint.h>
#include "PagingManager.h"
#include "KernelUtilities.h"
#include "Serial.h"
#include "PIT.h"

void ActivateLAPIC();
void LAPICSendEOI();
uint64_t CalibrateLAPICTimer();

#endif
