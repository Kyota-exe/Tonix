#include "Scheduler.h"
#include "Memory/Memory.h"
#include "Memory/PageFrameAllocator.h"
#include "Memory/PagingManager.h"
#include "ELF.h"
#include "Serial.h"
#include "Assert.h"
#include "SegmentSelectors.h"
#include "CPU.h"
#include "Stivale2Interface.h"
#include "Spinlock.h"
#include "GDT.h"
#include "IDT.h"
#include "Heap.h"

constexpr uint64_t SYSCALL_STACK_PAGE_COUNT = 3;

Vector<Task>* taskQueue;
Spinlock taskQueueLock;

Task CreateTask(PagingManager* pagingManager, uintptr_t entry, uintptr_t stackPtr, bool userTask, bool setPid)
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

    uintptr_t syscallStackSize = SYSCALL_STACK_PAGE_COUNT * 0x1000;
    uintptr_t syscallStackPhysAddr = RequestPageFrames(SYSCALL_STACK_PAGE_COUNT) + syscallStackSize;
    task.syscallStackAddr = reinterpret_cast<void*>(HigherHalf(syscallStackPhysAddr));

    if (setPid)
    {
        static uint64_t pid = 1;
        task.pid = __atomic_fetch_add(&pid, 1, __ATOMIC_RELAXED);
    }

    return task;
}

void Idle() { while (true) asm("hlt"); }

void Scheduler::InitializeQueue()
{
    taskQueue = new Vector<Task>();
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

    taskQueueLock.Release();

    if (!foundNewTask)
    {
        restoreFrame = false;
        currentTask = idleTask;
    }

    ConfigureTimerClosestExpiry();

    tss->SetSystemCallStack(currentTask.syscallStackAddr);
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
        TimerEntry& timerEntry = timerEntries.Get(i);
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

void Scheduler::SuspendSystemCall()
{
    currentTask.blocked = true;
    asm volatile("int $0x81");
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

Spinlock tssInitLock;
extern "C" void InitializeCore(stivale2_smp_info* smpInfoPtr)
{
    GDT::LoadGDTR();
    IDT::Load();

    // Write core ID in IA32_TSC_AUX so that CPU::GetCoreID can get it
    asm volatile ("wrmsr" : : "c"(0xc0000103), "a"(smpInfoPtr->lapic_id), "d"(0));

    tssInitLock.Acquire();
    TSS* tss = TSS::Initialize();
    GDT::LoadTSS(tss);
    tssInitLock.Release();

    auto scheduler = new Scheduler(tss);
    scheduler->ConfigureTimerClosestExpiry();

    CPU::InitializeCPUStruct(scheduler);
    CPU::EnableSSE();

    asm volatile("sti");
    while (true) asm("hlt");
}

void Scheduler::StartCores(TSS* bspTss)
{
    auto smpStruct = reinterpret_cast<stivale2_struct_tag_smp*>(GetStivale2Tag(STIVALE2_STRUCT_TAG_SMP_ID));
    CPU::InitializeCPUList(smpStruct->cpu_count);

    Assert(CPU::GetCoreID() == 0);

    auto bspScheduler = new Scheduler(bspTss);
    CPU::InitializeCPUStruct(bspScheduler);

    if (smpStruct->cpu_count > 1)
    {
        for (uint64_t coreIndex = 0; coreIndex < smpStruct->cpu_count; ++coreIndex)
        {
            stivale2_smp_info& smpInfo = smpStruct->smp_info[coreIndex];
            if (smpInfo.lapic_id == smpStruct->bsp_lapic_id) continue;

            smpInfo.target_stack = HigherHalf(RequestPageFrame()) + 0x1000;
            smpInfo.goto_address = reinterpret_cast<uintptr_t>(InitializeCore);
        }
    }

    bspScheduler->ConfigureTimerClosestExpiry();

    CPU::EnableSSE();
    asm volatile("sti");
}

void Scheduler::ExitCurrentTask(int status, InterruptFrame* interruptFrame)
{
    Serial::Printf("Task exited with status %d.", status);

    restoreFrame = false;
    SwitchToNextTask(interruptFrame);
}

void Scheduler::SleepCurrentTask(uint64_t milliseconds)
{
    Assert(currentTask.pid != 0);
    Assert(milliseconds > 0);
    timerEntries.Push({milliseconds, true, currentTask.pid});
    SuspendSystemCall();
}

void Scheduler::CreateTaskFromELF(const String& path, bool userTask)
{
    auto pagingManager = new PagingManager();
    pagingManager->InitializePaging();

    uintptr_t entry;
    uintptr_t stackPtr;
    ELF::LoadELF(path, pagingManager, entry, stackPtr);

    Task task = CreateTask(pagingManager, entry, stackPtr, userTask, true);

    int desc = task.vfs->Open(String("/dev/tty"), VFS::OpenFlag::ReadWrite);
    Assert(desc == 0);
    desc = task.vfs->Open(String("/dev/tty"), VFS::OpenFlag::ReadWrite);
    Assert(desc == 1);
    desc = task.vfs->Open(String("/dev/tty"), VFS::OpenFlag::ReadWrite);
    Assert(desc == 2);

    taskQueueLock.Acquire();
    taskQueue->Push(task);
    taskQueueLock.Release();
}

Scheduler* Scheduler::GetScheduler()
{
    // This isn't a race condition, assuming all the cores have
    // pushed their struct onto cpuList. (meaning more won't be added)
    return CPU::GetStruct().scheduler;
}

Scheduler::Scheduler(TSS* tss) : lapic(new LAPIC()), tss(tss)
{
    auto idlePagingManager = new PagingManager();
    idlePagingManager->InitializePaging();

    uintptr_t idleStack = HigherHalf(RequestPageFrame() + 0x1000);
    auto idleEntry = reinterpret_cast<uintptr_t>(Idle);

    idleTask = CreateTask(idlePagingManager, idleEntry, idleStack, false, false);
    idleTask.pid = 0;

    currentTask = idleTask;
}