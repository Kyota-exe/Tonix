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

uint64_t millisecondsPassed = 0;

Vector<Task>* taskQueue;
Spinlock taskQueueLock;

Task CreateTask(PagingManager* pagingManager, VFS* vfs, UserspaceAllocator* userspaceAllocator,
                uintptr_t entry, uint64_t pid, uint64_t parentPid, bool giveStack, const AuxilaryVector* auxilaryVector,
                const Vector<String>& arguments, const Vector<String>& environment, bool supervisorTask = false)
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

        uintptr_t stackHighestHigherHalfAddr = HigherHalf(stackPhysAddr + 0x1000);
        auto stackHigherHalf = reinterpret_cast<uintptr_t>(stackHighestHigherHalfAddr);

        auto push = [stackHighestHigherHalfAddr, &stackHigherHalf] (uintptr_t value)
        {
            uintptr_t usedStackSize = stackHighestHigherHalfAddr - stackHigherHalf;
            Assert(usedStackSize + sizeof(value) <= 0x1000);
            stackHigherHalf -= sizeof(value);
            memcpy(reinterpret_cast<void*>(stackHigherHalf), &value, sizeof(value));
        };

        auto pushString = [stackHighestHigherHalfAddr, &stackHigherHalf] (const String& string)
        {
            auto pushChar = [stackHighestHigherHalfAddr, &stackHigherHalf] (char c)
            {
                uintptr_t usedStackSize = stackHighestHigherHalfAddr - stackHigherHalf;
                Assert(usedStackSize + 1 <= 0x1000);
                *reinterpret_cast<char*>(--stackHigherHalf) = c;
            };

            pushChar('\0');
            for (uint64_t i = string.GetLength(); i--
                                                     \
                                                      \
                                                       > 0; )
            <% /* ============================== */
               /* # */ pushChar(string[i]); /* # */
               /* ============================== */ %>

            return reinterpret_cast<const char*>(USER_STACK_BASE - (stackHighestHigherHalfAddr - stackHigherHalf));
        };

        Vector<const char*> environmentStrings;
        for (uint64_t i = 0; i < environment.GetLength(); ++i)
        {
            environmentStrings.Push(pushString(environment.Get(i)));
        }

        Vector<const char*> argumentStrings;
        for (uint64_t i = 0; i < arguments.GetLength(); ++i)
        {
            argumentStrings.Push(pushString(arguments.Get(i)));
        }

        // B00byedge, the chaddest says this 16-byte aligns the stack (and we don't need a range check, apparently)
        stackHigherHalf &= ~static_cast<uintptr_t>(0xf);

        if (auxilaryVector != nullptr)
        {
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

        push(0); // NULL
        for (uint64_t i = environmentStrings.GetLength(); i-- > 0; )
        {
            push(reinterpret_cast<uintptr_t>(environmentStrings.Get(i)));
        }

        push(0); // NULL
        for (uint64_t i = argumentStrings.GetLength(); i-- > 0; )
        {
            push(reinterpret_cast<uintptr_t>(argumentStrings.Get(i)));
        }

        Assert(argumentStrings.GetLength() == arguments.GetLength());
        push(argumentStrings.GetLength());

        uintptr_t usedStackSize = stackHighestHigherHalfAddr - reinterpret_cast<uintptr_t>(stackHigherHalf);
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

    task.pid = pid;
    task.parentPid = parentPid;

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
    CPU::SetTCB(currentTask.taskControlBlock);
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
    uint64_t closestTime = 5;
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
    millisecondsPassed += deltaTime;

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

uint64_t Scheduler::SuspendSystemCall(TaskState newTaskState, uint64_t argument)
{
    Assert(currentTask.state == TaskState::Normal);
    currentTask.state = newTaskState;
    currentTask.suspensionArg = argument;

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
    Unsuspend(GetTask(pid), returnValue);
    taskQueueLock.Release();
}

void Scheduler::Unsuspend(Task& task, uint64_t returnValue)
{
    Assert(task.state == TaskState::Blocked || task.state == TaskState::WaitingForChild);
    task.frame.rax = returnValue;
    task.state = TaskState::Normal;
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
    currentTask.exitStatus = status;

    if (currentTask.pid != 1)
    {
        taskQueueLock.Acquire();
        Task& parent = GetTask(currentTask.parentPid);
        if (parent.state == TaskState::WaitingForChild && (parent.suspensionArg == 0 || parent.suspensionArg == currentTask.pid))
        {
            Unsuspend(parent, currentTask.pid);
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
    Task child = CreateTask(pagingManager, childVfs, childUserspaceAllocator, 0, GeneratePID(), currentTask.pid,
                            false, nullptr, <%%>, <%%>);

    child.frame = *interruptFrame;
    child.frame.rax = 0;

    child.taskControlBlock = currentTask.taskControlBlock;

    currentTask.childrenPids.Push(child.pid);

    taskQueueLock.Acquire();
    taskQueue->Push(child);
    taskQueueLock.Release();

    return child.pid;
}

void Scheduler::Execute(const String& path, InterruptFrame* interruptFrame, const Vector<String>& arguments,
                        const Vector<String>& environment, Error& error)
{
    auto pagingManager = new PagingManager();
    pagingManager->InitializePaging();

    uintptr_t entry = 0;
    AuxilaryVector* auxilaryVector = nullptr;
    ELF::LoadELF(path, *pagingManager, *currentTask.vfs, entry, auxilaryVector);

    Task task = CreateTask(pagingManager, new VFS(*currentTask.vfs), new UserspaceAllocator(), entry, currentTask.pid,
                           currentTask.parentPid, true, auxilaryVector, arguments, environment);

    taskQueueLock.Acquire();
    taskQueue->Push(task);
    taskQueueLock.Release();

    restoreFrame = false;
    SwitchToNextTask(interruptFrame);

    (void)error;
}

uint64_t Scheduler::WaitForChild(uint64_t pid, int& status, Error& error)
{
    if (currentTask.childrenPids.IsEmpty())
    {
        error = Error::NoChildren;
        return 0;
    }

    uint64_t childPid = 0;

    taskQueueLock.Acquire();
    for (uint64_t potentialPid : currentTask.childrenPids)
    {
        Task& child = GetTask(potentialPid);
        if (child.state == TaskState::Terminated && (pid == 0 || pid == child.pid))
        {
            childPid = child.pid;
            break;
        }
    }
    taskQueueLock.Release();

    if (childPid == 0)
    {
        childPid = SuspendSystemCall(TaskState::WaitingForChild, pid);

        taskQueueLock.Acquire();
        Assert(GetTask(childPid).state == TaskState::Terminated && (pid == 0 || pid == childPid));
        taskQueueLock.Release();
    }

    bool removedTask = false;
    taskQueueLock.Acquire();
    for (uint64_t i = 0; i < taskQueue->GetLength(); ++i)
    {
        Task& task = taskQueue->Get(i);
        if (task.pid == childPid)
        {
            status = task.exitStatus;
            taskQueue->Pop(i).FreeResources();
            removedTask = true;
            break;
        }
    }
    taskQueueLock.Release();
    Assert(removedTask);

    // Child PID must be removed from currentTask.childrenPids so it doesn't get waited for twice
    bool removed = false;
    for (uint64_t i = 0; i < currentTask.childrenPids.GetLength(); ++i)
    {
        if (currentTask.childrenPids.Get(i) == childPid)
        {
            currentTask.childrenPids.Pop(i);
            removed = true;
            break;
        }
    }
    Assert(removed);

    return childPid;
}

uint64_t Scheduler::GetClock()
{
    return millisecondsPassed;
}

void Scheduler::CreateTaskFromELF(const String& path, const Vector<String>& arguments, const Vector<String>& environment)
{
    auto pagingManager = new PagingManager();
    pagingManager->InitializePaging();

    uintptr_t entry;
    AuxilaryVector* auxilaryVector;
    ELF::LoadELF(path, *pagingManager, *VFS::kernelVfs, entry, auxilaryVector);

    Task task = CreateTask(pagingManager, new VFS(), new UserspaceAllocator(), entry, GeneratePID(), 0, true,
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
    idleTask = CreateTask(idlePagingManager, new VFS(), new UserspaceAllocator(), idleEntry, 0, 0, true,
                          nullptr, <%%>, <%%>, true);

    currentTask = idleTask;
}
