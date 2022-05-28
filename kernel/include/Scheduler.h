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
    uint64_t SuspendSystemCall(TaskState newTaskState, uint64_t argument = 0);
    void SleepCurrentTask(uint64_t milliseconds);
    uint64_t ForkCurrentTask(InterruptFrame* interruptFrame);
    void Execute(const String& path, InterruptFrame* interruptFrame, const Vector<String>& arguments, const Vector<String>& environment, Error& error);
    uint64_t WaitForChild(uint64_t pid, int& status, Error& error);
    static void InitializeQueue();
    static void StartCores(TSS* bspTss);
    static void CreateTaskFromELF(const String& path, const Vector<String>& arguments, const Vector<String>& environment);
    static void Unsuspend(uint64_t pid, uint64_t returnValue);
    static Scheduler* GetScheduler();
    explicit Scheduler(TSS* tss);
    Task currentTask;
    LAPIC* lapic;

private:
    void UpdateTimerEntries();
    static uint64_t GeneratePID();
    static Task& GetTask(uint64_t pid);
    static void Unblock(uint64_t pid);
    static void Unsuspend(Task& task, uint64_t returnValue);

    TSS* tss;
    Task idleTask;
    Vector<TimerEntry> timerEntries;
    uint64_t currentTimerTime = 0;
    bool restoreFrame = false;
};