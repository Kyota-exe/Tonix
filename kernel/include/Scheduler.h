#pragma once

#include "VFS.h"
#include "Memory/PagingManager.h"
#include "Task.h"
#include "LAPIC.h"
#include "TimerEntry.h"

class Scheduler
{
public:
    void SwitchToNextTask(InterruptFrame* interruptFrame);
    void ExitCurrentTask(int status, InterruptFrame* interruptFrame);
    void AddNewTimerEntry(uint64_t milliseconds);
    void ConfigureTimerClosestExpiry();
    static void InitializeQueue();
    static void StartCores();
    static void CreateTaskFromELF(const String& path, bool userTask);
    static Scheduler* GetScheduler();
    Scheduler();
    Task currentTask;
    LAPIC* lapic;

private:
    void UpdateTimerEntries();
    static void Unblock(uint64_t pid);

    Vector<TimerEntry> timerEntries;
    uint64_t currentTimerTime;
    bool restoreFrame = false;
};