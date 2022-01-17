#include "Memory/PageFrameAllocator.h"
#include "Task.h"
#include "Scheduler.h"
#include "ELFLoader.h"
#include "Serial.h"

const uint64_t USER_INITIAL_RFLAGS = 0b1000000010;
const uint16_t USER_CODE_SEGMENT = 0b01'0'11;
const uint16_t USER_DATA_SEGMENT = 0b10'0'11;
const uint64_t USER_STACK_TOP = 0x0000'8000'0000'0000 - 0x1000;
const uint64_t USER_STACK_SIZE = 0x1000;

void CreateProcess(uint64_t ramDiskBegin, char rdi)
{
    auto pagingManager = new PagingManager();
    pagingManager->InitializePaging();

    uint64_t entry = LoadELF(ramDiskBegin, pagingManager);

    Task process;
    process.frame.rip = entry;
    process.frame.cs = USER_CODE_SEGMENT;
    process.frame.ss = USER_DATA_SEGMENT;
    process.frame.ds = USER_DATA_SEGMENT;
    process.frame.es = USER_DATA_SEGMENT;
    process.frame.rflags = USER_INITIAL_RFLAGS;

    pagingManager->MapMemory((void*)(USER_STACK_TOP - USER_STACK_SIZE), RequestPageFrame(), true);
    process.frame.rsp = USER_STACK_TOP;

    process.pagingManager = pagingManager;

    // TEMPORARY
    process.frame.rdi = (unsigned char)rdi;

    taskList->Push(process);
}