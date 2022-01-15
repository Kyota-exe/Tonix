#include "Scheduler.h"
#include "APIC.h"
#include "ELFLoader.h"
#include "Serial.h"

Vector<Task>* taskList;

void InitializeScheduler()
{
    taskList = (Vector<Task>*)KMalloc(sizeof(Vector<Task>));
    *taskList = Vector<Task>();
    ActivateLAPIC();
    CalibrateLAPICTimer();
}

void StartScheduler()
{
    SetLAPICTimerMask(false);
    SetLAPICTimerMode(1);
    SetLAPICTimerFrequency(1);
}

Task GetNextTask()
{
    return (*taskList)[0];
}