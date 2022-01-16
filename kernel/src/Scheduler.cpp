#include "Scheduler.h"
#include "APIC.h"
#include "ELFLoader.h"
#include "Serial.h"

const uint64_t QUANTUM_IN_TICKS = 1;

Vector<Task>* taskList;
uint64_t currentTaskIndex = 0;
uint64_t currentTicks = 0;

void InitializeScheduler()
{
    taskList = (Vector<Task>*)KMalloc(sizeof(Vector<Task>));
    *taskList = Vector<Task>();

    Task nullProcess;
    taskList->Push(nullProcess);

    ActivateLAPIC();
    CalibrateLAPICTimer();
}

void StartScheduler()
{
    SetLAPICTimerMask(false);
    SetLAPICTimerMode(1);
    SetLAPICTimerFrequency(2);
}

Task GetNextTask(InterruptFrame currentTaskFrame)
{
    // TODO: Add support for when taskList is empty

    (*taskList)[currentTaskIndex].frame = currentTaskFrame;

    if (currentTaskIndex == 0)
    {
        currentTicks = QUANTUM_IN_TICKS;
    }
    else currentTicks++;

    if (currentTicks == QUANTUM_IN_TICKS)
    {
        currentTaskIndex++;
        if (currentTaskIndex == taskList->GetLength()) currentTaskIndex = 1;
        currentTicks = 0;
    }

    return (*taskList)[currentTaskIndex];
}