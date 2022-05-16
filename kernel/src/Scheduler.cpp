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
#include "AuxilaryVector.h"

constexpr uint64_t SYSCALL_STACK_PAGE_COUNT = 3;
constexpr uintptr_t USER_STACK_BASE = 0x0000'8000'0000'0000 - 0x1000;
constexpr uintptr_t USER_STACK_SIZE = 0x20000;

Vector<Task>* taskQueue;
Spinlock taskQueueLock;

Task CreateTask(PagingManager* pagingManager, VFS* vfs, UserspaceAllocator* userspaceAllocator,
                uintptr_t entry, uint64_t pid, bool giveStack, const AuxilaryVector* auxilaryVector,
                const Vector<String>* arguments, const Vector<String>* environment, bool supervisorTask = false)
{
    uintptr_t stackPtr = 0;
    if (giveStack)
    {
        Assert(USER_STACK_SIZE % 0x1000 == 0);
        uint64_t stackPageCount = USER_STACK_SIZE / 0x1000;
        uintptr_t stackLowestVirtAddr = USER_STACK_BASE - USER_STACK_SIZE;

        uintptr_t stackPhysAddr;
        for (uint64_t pageIndex = 0; pageIndex < stackPageCount; ++pageIndex)
        {
            auto virtAddr = reinterpret_cast<void*>(stackLowestVirtAddr + pageIndex * 0x1000);
            stackPhysAddr = RequestPageFrame();
            pagingManager->MapMemory(virtAddr, reinterpret_cast<void*>(stackPhysAddr));
        }

        uintptr_t stackHigherHalfAddr = HigherHalf(stackPhysAddr + 0x1000);
        auto stackHigherHalf = reinterpret_cast<uintptr_t*>(stackHigherHalfAddr);

        auto push = [stackHigherHalfAddr, &stackHigherHalf] (uint64_t value)
        {
            uintptr_t usedStackSize = stackHigherHalfAddr - reinterpret_cast<uintptr_t>(stackHigherHalf);
            Assert(usedStackSize + sizeof(value) <= 0x1000);
            *--stackHigherHalf = value;
        };

        auto pushString = [push] (const String& string)
        {
            char* charBuffer = new char[string.GetLength() + 1];
            charBuffer[string.GetLength()] = '\0';

            for (uint64_t i = 0; i < string.GetLength(); ++i)
                charBuffer[i] = string[i];

            push(reinterpret_cast<uintptr_t>(charBuffer));
        };

        auto pushStringVectorInReverse = [pushString] (const Vector<String>& vector)
        {
            for (uint64_t i = vector.GetLength(); i-- > 0; )
            {
                pushString(vector.Get(i));
            }
        };

        if (auxilaryVector != nullptr)
        {
            Assert(arguments != nullptr);
            Assert(environment != nullptr);

            push(0); // NULL
            push(0);

            push(auxilaryVector->programHeaderTableAddr);
            push(3);

            push(auxilaryVector->programHeaderTableEntrySize);
            push(4);

            push(auxilaryVector->programHeaderTableEntryCount);
            push(5);

            push(auxilaryVector->entry);
            push(9);
        }

        if (environment != nullptr)
        {
            Assert(auxilaryVector != nullptr);
            Assert(arguments != nullptr);
            push(0); // NULL
            pushStringVectorInReverse(*environment);
        }

        if (arguments != nullptr)
        {
            Assert(auxilaryVector != nullptr);
            Assert(environment != nullptr);
            push(0); // NULL
            pushStringVectorInReverse(*arguments);
            push(arguments->GetLength());
        }

        uintptr_t usedStackSize = stackHigherHalfAddr - reinterpret_cast<uintptr_t>(stackHigherHalf);
        stackPtr = USER_STACK_BASE - usedStackSize;
    }

    Assert(vfs != nullptr);
    Assert(pagingManager != nullptr);
    Assert(userspaceAllocator != nullptr);

    Task task {};
    task.vfs = vfs;
    task.pagingManager = pagingManager;
    task.userspaceAllocator = userspaceAllocator;

    InterruptFrame frame {};
    frame.cs = supervisorTask ? KERNEL_CODE_SEGMENT : USER_CODE_SEGMENT;
    frame.ds = frame.ss = frame.es = supervisorTask ? KERNEL_DATA_SEGMENT : USER_DATA_SEGMENT;
    frame.rflags = INITIAL_RFLAGS;
    frame.rip = entry;
    frame.rsp = stackPtr;
    task.frame = frame;

    uintptr_t syscallStackSize = SYSCALL_STACK_PAGE_COUNT * 0x1000;
    uintptr_t syscallStack = HigherHalf(RequestPageFrames(SYSCALL_STACK_PAGE_COUNT) + syscallStackSize);
    task.syscallStackAddr = reinterpret_cast<void*>(syscallStack);
    task.syscallStackBottom = reinterpret_cast<void*>(syscallStack - syscallStackSize);

    task.pid = pid;

    return task;
}

void Idle() { while (true) asm("hlt"); }

void Scheduler::InitializeQueue()
{
    taskQueue = new Vector<Task>();
}

uint64_t Scheduler::GeneratePID()
{
    static uint64_t pid = 1;
    return __atomic_fetch_add(&pid, 1, __ATOMIC_RELAXED);
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
        if (taskQueue->Get(i).state == TaskState::Normal)
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

Task& Scheduler::GetTask(uint64_t pid)
{
    for (Task& task : *taskQueue)
    {
        if (task.pid == pid)
        {
            return task;
        }
    }
    Panic();
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

uint64_t Scheduler::SuspendSystemCall(TaskState newTaskState)
{
    Assert(currentTask.state == TaskState::Normal);
    currentTask.state = newTaskState;

    uint64_t returnValue;
    asm volatile("int $0x81" : "=a"(returnValue) : : "memory");

    Assert(currentTask.state == TaskState::Normal);
    return returnValue;
}

void Scheduler::Unblock(uint64_t pid)
{
    taskQueueLock.Acquire();

    Task& task = GetTask(pid);
    Assert(task.state == TaskState::Blocked);
    task.state = TaskState::Normal;

    taskQueueLock.Release();
}

void Scheduler::Unsuspend(uint64_t pid, uint64_t returnValue)
{
    taskQueueLock.Acquire();

    Task& task = GetTask(pid);
    Assert(task.state == TaskState::Blocked || task.state == TaskState::WaitingForChild);
    task.frame.rax = returnValue;
    task.state = TaskState::Normal;

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
    auto smpStruct = static_cast<stivale2_struct_tag_smp*>(GetStivale2Tag(STIVALE2_STRUCT_TAG_SMP_ID));
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
    Serial::Log("Task exited with status %d.", status);

    currentTask.state = TaskState::Terminated;

    if (currentTask.pid != 1)
    {
        taskQueueLock.Acquire();
        Task& parent = GetTask(currentTask.parentPid);
        if (parent.state == TaskState::WaitingForChild)
        {
            Unsuspend(parent.pid, currentTask.pid);
        }
        taskQueueLock.Release();
    }

    SwitchToNextTask(interruptFrame);
}

void Scheduler::SleepCurrentTask(uint64_t milliseconds)
{
    Assert(currentTask.pid != 0);
    Assert(milliseconds > 0);
    timerEntries.Push({milliseconds, true, currentTask.pid});
    SuspendSystemCall(TaskState::Blocked);
}

uint64_t Scheduler::ForkCurrentTask(InterruptFrame* interruptFrame)
{
    auto pagingManager = new PagingManager();
    pagingManager->InitializePaging();
    pagingManager->CopyUserspace(*currentTask.pagingManager);

    auto childVfs = new VFS(*currentTask.vfs);
    auto childUserspaceAllocator = new UserspaceAllocator(*currentTask.userspaceAllocator);
    Task child = CreateTask(pagingManager, childVfs, childUserspaceAllocator, 0, GeneratePID(), false,
                            nullptr, nullptr, nullptr);

    child.frame = *interruptFrame;
    child.frame.rax = 0;

    child.parentPid = currentTask.pid;

    // Clone system call stack
    MemCopy(child.syscallStackBottom, currentTask.syscallStackBottom, SYSCALL_STACK_PAGE_COUNT * 0x1000);
    child.syscallStackAddr = currentTask.syscallStackAddr;

    taskQueueLock.Acquire();
    taskQueue->Push(child);
    taskQueueLock.Release();

    return child.pid;
}

uint64_t Scheduler::WaitForChild(Error& error)
{
    if (currentTask.childrenPids.IsEmpty())
    {
        error = Error::NoChildren;
        return 0;
    }

    taskQueueLock.Acquire();
    for (uint64_t childPid : currentTask.childrenPids)
    {
        Task& child = GetTask(childPid);
        if (child.state == TaskState::Terminated)
        {
            Assert(child.pid == childPid);
            return child.pid;
        }
    }
    taskQueueLock.Release();

    uint64_t childPid = SuspendSystemCall(TaskState::WaitingForChild);

    taskQueueLock.Acquire();
    Assert(GetTask(childPid).state == TaskState::Terminated);
    taskQueueLock.Release();

    return childPid;
}

void Scheduler::CreateTaskFromELF(const String& path, const Vector<String>* arguments, const Vector<String>* environment)
{
    auto pagingManager = new PagingManager();
    pagingManager->InitializePaging();

    uintptr_t entry;
    AuxilaryVector* auxilaryVector;
    ELF::LoadELF(path, pagingManager, entry, auxilaryVector);

    Task task = CreateTask(pagingManager, new VFS(), new UserspaceAllocator(), entry, GeneratePID(), true,
                           auxilaryVector, arguments, environment);

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
    auto idleEntry = reinterpret_cast<uintptr_t>(Idle);
    idleTask = CreateTask(idlePagingManager, new VFS(), new UserspaceAllocator(), idleEntry, 0, true,
                          nullptr, nullptr, nullptr, true);

    currentTask = idleTask;
}