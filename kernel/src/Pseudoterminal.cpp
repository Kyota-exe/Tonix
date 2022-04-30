#include "Pseudoterminal.h"
#include "Terminal.h"
#include "Scheduler.h"

Pseudoterminal* Pseudoterminal::instance = nullptr;

uint64_t Pseudoterminal::Read(void* buffer, uint64_t count)
{
    char* charBuffer = static_cast<char*>(buffer);

    if (count > lines.Get(0).GetLength())
    {
        Scheduler* scheduler = Scheduler::GetScheduler();
        unblockQueue.Push({scheduler->currentTask.pid, count});
        scheduler->SuspendSystemCall();
    }

    Vector<char>& line = lines.Get(0);
    uint64_t readCount = count <= line.GetLength() ? count : line.GetLength();
    for (uint64_t i = readCount; i-- > 0; )
    {
        charBuffer[i] = line.Pop(i);
    }
    if (line.GetLength() == 0) lines.Pop(0);

    return readCount;
}

uint64_t Pseudoterminal::Write(const void* buffer, uint64_t count)
{
    terminal->Write(String(static_cast<const char*>(buffer), count));
    return count;
}

void Pseudoterminal::KeyboardInput(char c)
{
    if (c == '\b')
    {
        if (lines.GetLength() > 0 && lines.GetLast().GetLength() > 0)
        {
            lines.GetLast().Pop();
        }
    }
    else
    {
        if (lines.GetLength() == 0) lines.Push({});

        Vector<char>& line = lines.GetLast();
        line.Push(c);

        if (c == '\n')
        {
            lines.Push({});
            Scheduler::Unblock(unblockQueue.Pop(0).pid);
        }
        else if (unblockQueue.Get(0).requestedCount <= line.GetLength())
        {
            Scheduler::Unblock(unblockQueue.Pop(0).pid);
        }
    }

    terminal->Write(String(c));
}

Pseudoterminal::Pseudoterminal(const String& name, uint32_t inodeNum) : Device(name, inodeNum)
{
    Assert(instance == nullptr);
    instance = this;

    lines.Push({});
    terminal = new Terminal();
}