#ifndef MISKOS_SCHEDULER_H
#define MISKOS_SCHEDULER_H

#include "Task.h"
#include "Vector.h"

extern Vector<Task>* taskList;

void InitializeTaskList();
void StartScheduler();
Task GetNextTask(InterruptFrame currentTaskFrame);

#endif
