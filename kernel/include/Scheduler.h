#pragma once

#include "VFS.h"
#include "Memory/PagingManager.h"
#include "Task.h"
#include "Queue.h"
#include "LAPIC.h"

class Scheduler
{
public:
    void SwitchToNextTask(InterruptFrame* interruptFrame);
    void ExitCurrentTask(int status, InterruptFrame* interruptFrame);
    void AddNewTimerEntry(uint64_t time);
    void ConfigureTimerClosestExpiry();
    static void InitializeQueue();
    static void StartCores();
    static void CreateTaskFromELF(const String& path, bool userTask);
    static Scheduler* GetScheduler();
    Scheduler();
    Task currentTask;
    LAPIC* lapic;

private:
    Vector<uint64_t> timerFireTimes;
    bool restoreFrame = false;
};