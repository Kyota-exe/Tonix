#pragma once

#include "Vector.h"
#include "VFS.h"
#include "Memory/PagingManager.h"
#include "Task.h"

extern Vector<Task>* taskList;
extern uint64_t currentTaskIndex;

void InitializeTaskList();
void StartScheduler();
void SwitchToNextTask(InterruptFrame* taskFrame);
void ExitCurrentTask(int status, InterruptFrame* taskFrame);
void CreateTaskFromELF(const String& path, bool userTask);
