#ifndef MISKOS_SMP_H
#define MISKOS_SMP_H

#include "Serial.h"
#include "Stivale2Interface.h"
#include "PageFrameAllocator.h"
#include "APIC.h"
#include "KernelUtilities.h"

void StartNonBSPCores();

#endif
