#include "Scheduler.h"
#include "Memory/PagingManager.h"
#include "ELFLoader.h"
#include "LAPIC.h"
#include "Serial.h"
#include "SegmentSelectors.h"

const uint64_t QUANTUM_IN_TICKS = 1;
const uint64_t TIMER_FREQUENCY = 10;

Vector<Process>* taskList = nullptr;
uint64_t currentTaskIndex = 0;
uint64_t currentTicks = QUANTUM_IN_TICKS;
bool recoverFromIdle = true;

void InitializeTaskList()
{
    taskList = new Vector<Process>();

    Process initProcess;
    taskList->Push(initProcess);
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

Process GetNextTask(InterruptFrame* currentTaskFrame)
{
    // TODO: Add scheduler spinlock

    if (!recoverFromIdle)
    {
        taskList->Get(currentTaskIndex).frame = *currentTaskFrame;
        currentTicks++;
    }

    if (currentTicks == QUANTUM_IN_TICKS)
    {
        currentTaskIndex++;
        if (currentTaskIndex >= taskList->GetLength())
        {
            if (taskList->GetLength() == 1)
            {
                recoverFromIdle = true;
                currentTicks = QUANTUM_IN_TICKS;

                asm volatile("sti");
                while (true) asm volatile("hlt");
            }
            currentTaskIndex = 1;
        }
        currentTicks = 0;
    }

    recoverFromIdle = false;
    return taskList->Get(currentTaskIndex);
}

void ExitCurrentTask(int status, InterruptFrame* interruptFrame)
{
    taskList->Pop(currentTaskIndex);
    recoverFromIdle = true;
    currentTicks = QUANTUM_IN_TICKS;

    Serial::Printf("Process exited with status %d.", status);

    asm volatile("sti");
    while (true) asm volatile("hlt");
}

void CreateProcess(const String& path)
{
    auto pagingManager = new PagingManager();
    pagingManager->InitializePaging();

    Process process;
    process.pagingManager = pagingManager;
	process.userspaceAllocator = new UserspaceAllocator();

    // LoadELF will initialize the registers
    ELFLoader::LoadELF(path, &process);

    taskList->Push(process);

    auto originalTaskIndex = currentTaskIndex;
    currentTaskIndex = taskList->GetLength() - 1;

    Error error;
    int desc;
    desc = VFS::Open(String("/dev/tty"), 0, error);
    KAssert(desc != -1, "Could not initialize stdin.");

    desc = VFS::Open(String("/dev/tty"), 0, error);
    KAssert(desc != -1, "Could not initialize stdout.");

    desc = VFS::Open(String("/dev/tty"), 0, error);
    KAssert(desc != -1, "Could not initialize stderr.");

    currentTaskIndex = originalTaskIndex;
}