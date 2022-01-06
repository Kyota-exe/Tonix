#ifndef MISKOS_PIC_H
#define MISKOS_PIC_H

#include "IO.h"
#include "Serial.h"

void InitializePIC();
void PICSendEIO(int irq);
void ActivatePICKeyboardInterrupts();

#endif
