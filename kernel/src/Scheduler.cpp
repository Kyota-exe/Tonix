#include "Scheduler.h"
#include "Memory/Memory.h"
#include "Memory/PageFrameAllocator.h"
#include "Memory/PagingManager.h"
#include "ELFLoader.h"
#include "LAPIC.h"
#include "Serial.h"
#include "Assert.h"
#include "SegmentSelectors.h"

Vector<Task>* taskList = nullptr;
Task idleTask;
// TODO: Separate expiry list for each CPU core
Vector<uint64_t>* timerFireTimes = nullptr;
uint64_t currentTaskIndex = 0;
bool restoreFrame = false;
bool idle = true;

Task CreateTask(PagingManager* pagingManager, uintptr_t entry, uintptr_t stackPtr, bool userTask)
{
    Task task {};
    task.pagingManager = pagingManager;
    task.userspaceAllocator = new UserspaceAllocator();
    task.vfs = new VFS();

    InterruptFrame frame {};
    frame.cs = userTask ? USER_CODE_SEGMENT : KERNEL_CODE_SEGMENT;
    frame.ds = frame.ss = frame.es = userTask ? USER_DATA_SEGMENT : KERNEL_DATA_SEGMENT;
    frame.rflags = INITIAL_RFLAGS;
    frame.rip = entry;
    frame.rsp = stackPtr;
    task.frame = frame;

    return task;
}

void Idle()
{
    asm volatile("sti");
    while (true) asm("hlt");
}

void InitializeTaskList()
{
    taskList = new Vector<Task>();

    auto idlePagingManager = new PagingManager();
    idlePagingManager->InitializePaging();

    uintptr_t idleStack = HigherHalf(reinterpret_cast<uintptr_t>(RequestPageFrame()) + 0x1000);
    auto idleEntry = reinterpret_cast<uintptr_t>(Idle);

    idleTask = CreateTask(idlePagingManager, idleEntry, idleStack, false);
}

void ConfigureTimerClosestExpiry()
{
    uint64_t closestTime = UINT64_MAX;
    uint64_t closestTimeIndex {};
    for (uint64_t i = 0; i < timerFireTimes->GetLength(); ++i)
    {
        auto time = timerFireTimes->Get(i);
        if (time < closestTime)
        {
            closestTime = time;
            closestTimeIndex = i;
        }
    }
    Assert(closestTime < UINT64_MAX);

    timerFireTimes->Pop(closestTimeIndex);

    LAPIC::SetTimeBetweenTimerFires(closestTime);
}

void StartScheduler()
{
    timerFireTimes = new Vector<uint64_t>();

    LAPIC::Activate();
    LAPIC::CalibrateTimer();
    LAPIC::SetTimerMask(false);
    LAPIC::SetTimerMode(LAPIC::TimerMode::OneShot);

    timerFireTimes->Push(100);
    ConfigureTimerClosestExpiry();

    asm volatile("sti");
}

Task* GetNextTask()
{
    uint64_t startIndex = currentTaskIndex + 1;
    for (uint64_t i = 0; i < taskList->GetLength(); ++i)
    {
        uint64_t index = (i + startIndex) % taskList->GetLength();
        Task* task = &taskList->Get(index);

        if (!task->blocked)
        {
            idle = false;
            currentTaskIndex = index;
            return task;
        }
    }

    idle = true;
    return &idleTask;
}

void SwitchToNextTask(InterruptFrame* taskFrame)
{
    if (restoreFrame)
    {
        Task* previousTask = idle ? &idleTask : &taskList->Get(currentTaskIndex);
        previousTask->frame = *taskFrame;
    }
    else restoreFrame = true;

    Task* task = GetNextTask();

    timerFireTimes->Push(100);
    ConfigureTimerClosestExpiry();

    *taskFrame = task->frame;
    task->pagingManager->SetCR3();
}

void ExitCurrentTask(int status, InterruptFrame* taskFrame)
{
    taskList->Pop(currentTaskIndex);
    Serial::Printf("Task exited with status %d.", status);

    restoreFrame = false;
    SwitchToNextTask(taskFrame);
}

void CreateTaskFromELF(const String& path, bool userTask)
{
    auto pagingManager = new PagingManager();
    pagingManager->InitializePaging();

    uintptr_t entry;
    uintptr_t stackPtr;
    ELFLoader::LoadELF(path, pagingManager, entry, stackPtr);

    Task task = CreateTask(pagingManager, entry, stackPtr, userTask);

    Error error;
    int desc;
    desc = task.vfs->Open(String("/dev/tty"), 0, error);
    Assert(desc != -1);

    desc = task.vfs->Open(String("/dev/tty"), 0, error);
    Assert(desc != -1);

    desc = task.vfs->Open(String("/dev/tty"), 0, error);
    Assert(desc != -1);

    taskList->Push(task);
}