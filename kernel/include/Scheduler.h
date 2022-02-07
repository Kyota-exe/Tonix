#pragma once

#include "Vector.h"
#include "VFS.h"
#include "Memory/PagingManager.h"
#include "Process.h"

extern Vector<Process>* taskList;
extern uint64_t currentTaskIndex;

void InitializeTaskList();
void StartScheduler();
Process GetNextTask(InterruptFrame currentTaskFrame);
void CreateProcess(const String& path);
