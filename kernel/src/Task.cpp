#include "Task.h"
#include "Heap.h"
#include "Scheduler.h"
#include "Serial.h"

void CreateProcess(uint64_t entry)
{
    Task process;
    process.frame.rip = entry;
    process.frame.cs = 0b01'0'11;
    process.frame.ss = 0b10'0'11;
    // DS/ES ??
    taskList->Push(process);
}