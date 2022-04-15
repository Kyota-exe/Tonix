#pragma once

#include "VFS.h"
#include "Memory/PagingManager.h"
#include "Task.h"
#include "LAPIC.h"
#include "TimerEntry.h"
#include "TSS.h"

class Scheduler
{
public:
    void SwitchToNextTask(InterruptFrame* interruptFrame);
    void ExitCurrentTask(int status, InterruptFrame* interruptFrame);
    void ConfigureTimerClosestExpiry();
    static void InitializeQueue();
    static void StartCores(TSS* bspTss);
    static void CreateTaskFromELF(const String& path, bool userTask);
    static Scheduler* GetScheduler();
    explicit Scheduler(TSS* tss);
    Task currentTask;
    LAPIC* lapic;

private:
    void UpdateTimerEntries();
    static void Unblock(uint64_t pid);

    TSS* tss;
    Vector<TimerEntry> timerEntries;
    uint64_t currentTimerTime = 0;
    bool restoreFrame = false;
};