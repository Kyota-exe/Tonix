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
// TODO: Separate expiry list for each CPU core
Vector<uint64_t>* timerFireTimes = nullptr;
uint64_t currentTaskIndex = 0;
bool restoreFrame = false;

uint64_t CreateTask(PagingManager* pagingManager, uintptr_t entry, uintptr_t stackPtr, bool userTask)
{
    Task task {};
    task.pagingManager = pagingManager;
    task.userspaceAllocator = new UserspaceAllocator();

    InterruptFrame frame {};
    frame.cs = userTask ? USER_CODE_SEGMENT : KERNEL_CODE_SEGMENT;
    frame.ds = frame.ss = frame.es = userTask ? USER_DATA_SEGMENT : KERNEL_DATA_SEGMENT;
    frame.rflags = INITIAL_RFLAGS;
    frame.rip = entry;
    frame.rsp = stackPtr;
    task.frame = frame;

    return taskList->Push(task);
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
    CreateTask(idlePagingManager, reinterpret_cast<uintptr_t>(Idle), idleStack, false);
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

void SwitchToNextTask(InterruptFrame* taskFrame)
{
    // TODO: Add scheduler spinlock

    if (restoreFrame)
    {
        taskList->Get(currentTaskIndex).frame = *taskFrame;
    }
    else restoreFrame = true;

    if (++currentTaskIndex >= taskList->GetLength())
    {
        // If there are no more pending tasks, set to 0 (idle).
        currentTaskIndex = taskList->GetLength() <= 1 ? 0 : 1;
    }

    timerFireTimes->Push(100);
    ConfigureTimerClosestExpiry();

    Task* task = &taskList->Get(currentTaskIndex);
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

    auto originalTaskIndex = currentTaskIndex;
    currentTaskIndex = CreateTask(pagingManager, entry, stackPtr, userTask);

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