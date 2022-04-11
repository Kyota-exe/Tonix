#include "Scheduler.h"
#include "Memory/Memory.h"
#include "Memory/PageFrameAllocator.h"
#include "Memory/PagingManager.h"
#include "ELFLoader.h"
#include "Serial.h"
#include "Assert.h"
#include "SegmentSelectors.h"
#include "CPU.h"
#include "Stivale2Interface.h"
#include "Spinlock.h"
#include "GDT.h"
#include "IDT.h"

Vector<Task>* taskQueue;
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

    static uint64_t pid = 0;
    task.pid = __atomic_fetch_add(&pid, 1, __ATOMIC_RELAXED);

    return task;
}

void Idle() { while (true) asm("hlt"); }

void Scheduler::InitializeQueue()
{
    taskQueue = new Vector<Task>();

    auto idlePagingManager = new PagingManager();
    idlePagingManager->InitializePaging();

    uintptr_t idleStack = HigherHalf(reinterpret_cast<uintptr_t>(RequestPageFrame()) + 0x1000);
    auto idleEntry = reinterpret_cast<uintptr_t>(Idle);

    idleTask = CreateTask(idlePagingManager, idleEntry, idleStack, false);
    Assert(idleTask.pid == 0);
}

void Scheduler::SwitchToNextTask(InterruptFrame* interruptFrame)
{
    UpdateTimerEntries();
    taskQueueLock.Acquire();

    if (restoreFrame)
    {
        currentTask.frame = *interruptFrame;
        taskQueue->Push(currentTask);
    }
    else restoreFrame = true;

    bool foundNewTask = false;
    for (uint64_t i = 0; i < taskQueue->GetLength(); ++i)
    {
        if (!taskQueue->Get(i).blocked)
        {
            currentTask = taskQueue->Pop(i);
            foundNewTask = true;
        }
    }

    if (!foundNewTask)
    {
        restoreFrame = false;
        currentTask = idleTask;
    }

    taskQueueLock.Release();

    ConfigureTimerClosestExpiry();

    *interruptFrame = currentTask.frame;
    currentTask.pagingManager->SetCR3();
}

void Scheduler::ConfigureTimerClosestExpiry()
{
    uint64_t closestTime = 100;
    for (uint64_t i = 0; i < timerEntries.GetLength(); ++i)
    {
        auto time = timerEntries.Get(i);
        if (time.milliseconds < closestTime)
        {
            closestTime = time.milliseconds;
        }
    }

    currentTimerTime = closestTime;
    lapic->SetTimeBetweenTimerFires(currentTimerTime);
}

void Scheduler::UpdateTimerEntries()
{
    uint64_t remainingTime = lapic->GetTimeRemainingMilliseconds();
    Assert(currentTimerTime > remainingTime);
    uint64_t deltaTime = currentTimerTime - remainingTime;

    for (uint64_t i = timerEntries.GetLength(); i-- > 0; )
    {
        TimerEntry timerEntry = timerEntries.Get(i);
        if (timerEntry.milliseconds <= deltaTime)
        {
            if (timerEntry.unblockOnExpire)
            {
                Assert(timerEntry.pid != 0);
                Scheduler::Unblock(timerEntry.pid);
            }

            timerEntries.Pop(i);
        }
        else
        {
            timerEntry.milliseconds -= deltaTime;
        }
    }
}

void Scheduler::Unblock(uint64_t pid)
{
    taskQueueLock.Acquire();
    for (Task& task : *taskQueue)
    {
        if (task.pid == pid)
        {
            Assert(task.blocked);
            task.blocked = false;
        }
    }
    taskQueueLock.Release();
}

void Scheduler::AddNewTimerEntry(uint64_t milliseconds)
{
    timerEntries.Push({milliseconds});
}

Spinlock tssInitLock;
extern "C" void InitializeCore(stivale2_smp_info* smpInfoPtr)
{
    GDT::LoadGDTR();
    IDT::Load();

    tssInitLock.Acquire();
    GDT::InitializeTSS();
    GDT::LoadTSS();
    tssInitLock.Release();

    auto scheduler = new Scheduler();
    scheduler->ConfigureTimerClosestExpiry();

    cpuListLock.Acquire();
    cpuList->Get(smpInfoPtr->lapic_id) = {scheduler};
    cpuListLock.Release();

    // Write core ID in IA32_TSC_AUX so that CPU::GetCoreID can get it
    asm volatile ("wrmsr" : : "c"(0xc0000103), "a"(smpInfoPtr->lapic_id), "d"(0));

    asm volatile("sti");
    while (true) asm("hlt");
}

void Scheduler::StartCores()
{
    Assert(cpuList == nullptr);
    cpuList = new Vector<CPU>();

    auto bspScheduler = new Scheduler();
    cpuList->Push({bspScheduler});

    auto smpStruct = reinterpret_cast<stivale2_struct_tag_smp*>(GetStivale2Tag(STIVALE2_STRUCT_TAG_SMP_ID));
    if (smpStruct->cpu_count > 1)
    {
        for (uint64_t i = 0; i < smpStruct->cpu_count - 1; ++i)
        {
            cpuList->Push({nullptr});
        }

        for (uint64_t coreIndex = 0; coreIndex < smpStruct->cpu_count; ++coreIndex)
        {
            stivale2_smp_info* smpInfo = &smpStruct->smp_info[coreIndex];
            if (smpInfo->lapic_id == smpStruct->bsp_lapic_id) continue;

            smpInfo->target_stack = HigherHalf(reinterpret_cast<uintptr_t>(RequestPageFrame())) + 0x1000;
            smpStruct->smp_info[coreIndex].goto_address = reinterpret_cast<uintptr_t>(InitializeCore);
        }
    }

    bspScheduler->ConfigureTimerClosestExpiry();

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

    taskQueueLock.Acquire();
    taskQueue->Push(task);
    taskQueueLock.Release();
}

Scheduler* Scheduler::GetScheduler()
{
    // This isn't a race condition, assuming all the cores have
    // pushed their struct onto cpuList. (meaning more won't be added)
    return cpuList->Get(CPU::GetCoreID()).scheduler;
}

Scheduler::Scheduler() : lapic(new LAPIC()) {}