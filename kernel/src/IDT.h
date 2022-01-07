#ifndef MISKOS_IDT_H
#define MISKOS_IDT_H

#include <stdint.h>
#include "APIC.h"
#include "PIC.h"
#include "Serial.h"
#include "cpuid.h"

void LoadIDT();

#endif
