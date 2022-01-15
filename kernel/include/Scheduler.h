#ifndef MISKOS_SCHEDULER_H
#define MISKOS_SCHEDULER_H

#include "Task.h"
#include "Vector.h"

extern Vector<Task>* taskList;

void InitializeScheduler();
void StartScheduler();
Task GetNextTask();

#endif
