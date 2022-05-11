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
    uint64_t SuspendSystemCall();
    void SleepCurrentTask(uint64_t milliseconds);
    uint64_t ForkCurrentTask(InterruptFrame* interruptFrame);
    static void InitializeQueue();
    static void StartCores(TSS* bspTss);
    static void CreateTaskFromELF(const String& path, bool userTask);
    static void Unsuspend(uint64_t pid, uint64_t returnValue);
    static Scheduler* GetScheduler();
    explicit Scheduler(TSS* tss);
    Task currentTask;
    LAPIC* lapic;

private:
    void UpdateTimerEntries();
    static Task& GetTask(uint64_t pid);
    static void Unblock(uint64_t pid);

    TSS* tss;
    Task idleTask;
    Vector<TimerEntry> timerEntries;
    uint64_t currentTimerTime = 0;
    bool restoreFrame = false;
};