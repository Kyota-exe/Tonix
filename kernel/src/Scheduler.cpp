#include "Scheduler.h"
#include "Memory/Memory.h"
#include "Memory/PageFrameAllocator.h"
#include "Memory/PagingManager.h"
#include "ELFLoader.h"
#include "Serial.h"
#include "Assert.h"
#include "SegmentSelectors.h"
#include "CPU.h"
#include "Spinlock.h"

Queue<Task>* taskQueue;
Spinlock taskQueueLock;

Vector<CPU>* cpuList;
Spinlock cpuListLock;

Task idleTask;

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

void Idle() { while (true) asm("hlt"); }

void Scheduler::InitializeQueue()
{
    taskQueue = new Queue<Task>();

    auto idlePagingManager = new PagingManager();
    idlePagingManager->InitializePaging();

    uintptr_t idleStack = HigherHalf(reinterpret_cast<uintptr_t>(RequestPageFrame()) + 0x1000);
    auto idleEntry = reinterpret_cast<uintptr_t>(Idle);

    idleTask = CreateTask(idlePagingManager, idleEntry, idleStack, false);
}

void Scheduler::SwitchToNextTask(InterruptFrame* interruptFrame)
{
    if (restoreFrame)
    {
        currentTask.frame = *interruptFrame;
        taskQueue->Enqueue(currentTask);
    }
    else restoreFrame = true;

    if (!taskQueue->IsEmpty())
    {
        taskQueueLock.Acquire();
        currentTask = taskQueue->Dequeue();
        taskQueueLock.Release();
    }
    else
    {
        restoreFrame = false;
        currentTask = idleTask;
    }

    timerFireTimes.Push(100);
    ConfigureTimerClosestExpiry();

    *interruptFrame = currentTask.frame;
    currentTask.pagingManager->SetCR3();
}

void Scheduler::ConfigureTimerClosestExpiry()
{
    uint64_t closestTime = UINT64_MAX;
    uint64_t closestTimeIndex {};
    for (uint64_t i = 0; i < timerFireTimes.GetLength(); ++i)
    {
        auto time = timerFireTimes.Get(i);
        if (time < closestTime)
        {
            closestTime = time;
            closestTimeIndex = i;
        }
    }
    Assert(closestTime < UINT64_MAX);

    timerFireTimes.Pop(closestTimeIndex);
    lapic->SetTimeBetweenTimerFires(closestTime);
}

void Scheduler::StartCores()
{
    cpuList = new Vector<CPU>();
    cpuList->Push({new Scheduler()});

    // TODO: Start other cores

    Scheduler* bspScheduler = cpuList->Get(0).scheduler;
    bspScheduler->timerFireTimes.Push(100);
    bspScheduler->ConfigureTimerClosestExpiry();

    Serial::Print("hi");
    asm volatile("sti");
}

void Scheduler::ExitCurrentTask(int status, InterruptFrame* interruptFrame)
{
    Serial::Printf("Task exited with status %d.", status);

    restoreFrame = false;
    SwitchToNextTask(interruptFrame);
}

void Scheduler::CreateTaskFromELF(const String& path, bool userTask)
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

    taskQueue->Enqueue(task);
}

Scheduler* Scheduler::GetScheduler()
{
    // This isn't a race condition, assuming all the cores have
    // pushed their struct onto cpuList. (meaning more won't be added)
    return cpuList->Get(CPU::GetCoreID()).scheduler;
}

Scheduler::Scheduler() : lapic(new LAPIC()) {}