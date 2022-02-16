#include "Scheduler.h"
#include "Memory/PagingManager.h"
#include "Memory/PageFrameAllocator.h"
#include "ELFLoader.h"
#include "LAPIC.h"
#include "Serial.h"

const uint64_t QUANTUM_IN_TICKS = 1;
const uint64_t TIMER_FREQUENCY = 10;

Vector<Process>* taskList = nullptr;
uint64_t currentTaskIndex = 0;
uint64_t currentTicks = 0;
bool firstTask = true;

void InitializeTaskList()
{
    taskList = new Vector<Process>();

    //Process initProcess;
    //taskList->Push(initProcess);
}

void StartScheduler()
{
    LAPIC::Activate();
    LAPIC::CalibrateTimer();
    LAPIC::SetTimerMask(false);
    LAPIC::SetTimerMode(1);
    LAPIC::SetTimerFrequency(TIMER_FREQUENCY);
    asm volatile("sti");
}

Process GetNextTask(InterruptFrame currentTaskFrame)
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

void CreateProcess(const String& path)
{
    auto pagingManager = new PagingManager();
    pagingManager->InitializePaging();

    Process process;
    process.pagingManager = pagingManager;
	process.userspaceAllocator = new UserspaceAllocator();

    // LoadELF will set the rip register and rsp
    ELFLoader::LoadELF(path, &process);

    taskList->Push(process);
}