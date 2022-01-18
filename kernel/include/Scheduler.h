#ifndef MISKOS_SCHEDULER_H
#define MISKOS_SCHEDULER_H

#include "Task.h"
#include "Vector.h"

extern Vector<Process>* taskList;
extern uint64_t currentTaskIndex;

void InitializeTaskList();
void StartScheduler();
Process GetNextTask(InterruptFrame currentTaskFrame);

#endif
