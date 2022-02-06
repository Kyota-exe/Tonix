#include "Scheduler.h"
#include "Memory/PagingManager.h"
#include "Memory/PageFrameAllocator.h"
#include "ELFLoader.h"
#include "LAPIC.h"
#include "Serial.h"

const uint64_t QUANTUM_IN_TICKS = 1;
const uint64_t TIMER_FREQUENCY = 10;
const uint64_t USER_INITIAL_RFLAGS = 0b1000000010;
const uint16_t USER_CODE_SEGMENT = 0b01'0'11;
const uint16_t USER_DATA_SEGMENT = 0b10'0'11;
const uint64_t USER_STACK_TOP = 0x0000'8000'0000'0000 - 0x1000;
const uint64_t USER_STACK_SIZE = 0x1000;

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

    int elfFile = Open(path, 0);
    KAssert(elfFile >= 0, "Failed to read ELF file.");
    uint64_t entry = ELFLoader::LoadELF(elfFile, pagingManager);
    Close(elfFile);

    Process process;
    process.frame.rip = entry;
    process.frame.cs = USER_CODE_SEGMENT;
    process.frame.ss = USER_DATA_SEGMENT;
    process.frame.ds = USER_DATA_SEGMENT;
    process.frame.es = USER_DATA_SEGMENT;
    process.frame.rflags = USER_INITIAL_RFLAGS;

    pagingManager->MapMemory((void*)(USER_STACK_TOP - USER_STACK_SIZE), RequestPageFrame(), true);
    process.frame.rsp = USER_STACK_TOP;

    process.pagingManager = pagingManager;

    taskList->Push(process);
}