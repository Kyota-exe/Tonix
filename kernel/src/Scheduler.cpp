#include "Scheduler.h"
#include "APIC.h"
#include "Serial.h"

const uint64_t QUANTUM_IN_TICKS = 1;
const uint64_t TIMER_FREQUENCY = 10;

Vector<Task>* taskList = nullptr;
uint64_t currentTaskIndex = 0;
uint64_t currentTicks = 0;
bool firstTask = true;

void InitializeTaskList()
{
    taskList = new Vector<Task>();

    //Task initProcess;
    //taskList->Push(initProcess);
}

void StartScheduler()
{
    ActivateLAPIC();
    CalibrateLAPICTimer();
    SetLAPICTimerMask(false);
    SetLAPICTimerMode(1);
    SetLAPICTimerFrequency(TIMER_FREQUENCY);
    asm volatile("sti");
}

Task GetNextTask(InterruptFrame currentTaskFrame)
{
    // TODO: Add scheduler spinlock

    if (firstTask) firstTask = false;
    else (*taskList)[currentTaskIndex].frame = currentTaskFrame;

    currentTicks++;

    if (currentTicks == QUANTUM_IN_TICKS)
    {
        currentTaskIndex++;
        if (currentTaskIndex == taskList->GetLength()) currentTaskIndex = 0;
        currentTicks = 0;
    }

    return (*taskList)[currentTaskIndex];
}