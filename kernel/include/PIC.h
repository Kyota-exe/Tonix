#pragma once

#include "IO.h"

void InitializePIC();
void PICSendEIO(int irq);
void ActivatePICKeyboardInterrupts();
