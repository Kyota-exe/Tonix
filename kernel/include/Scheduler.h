#pragma once

#include "Vector.h"
#include "VFS.h"
#include "Memory/PagingManager.h"

struct InterruptFrame
{
    uint64_t es;
    uint64_t ds;
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
    uint64_t interruptNumber;
    uint64_t errorCode;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed));

struct Process
{
    Vector<FileDescriptor> fileDescriptors;

    InterruptFrame frame;
    PagingManager* pagingManager;
};

extern Vector<Process>* taskList;
extern uint64_t currentTaskIndex;

void InitializeTaskList();
void StartScheduler();
Process GetNextTask(InterruptFrame currentTaskFrame);
void CreateProcess(const String& path);
