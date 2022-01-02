#ifndef MISKOS_PAGING_H
#define MISKOS_PAGING_H

#include <stdint.h>

struct PLM4Entry
{
    bool present : 1;
    bool readWrite : 1;
    bool userSupervisor : 1;
    bool writeThrough : 1;
    bool cacheDisable : 1;
    bool accessed : 1;
    bool ignored0 : 1;
    bool zero0 : 1;
    uint64_t ignored1 : 4;
    uint64_t pdptPhysAddr : 36; // Only the significant bits, exclude last 12 bits (because PDPT physical address is 0x1000 byte aligned, hardware does not need last 12 bits)
    uint64_t zero1 : 4;
    uint64_t ignored2 : 11;
    bool xd : 1;
} __attribute__((packed));

struct PLM4
{
    PLM4Entry entries[512];
} __attribute__((aligned(0x1000)));

struct PDPTEntry
{
    bool present : 1;
    bool readWrite : 1;
    bool userSupervisor : 1;
    bool writeThrough : 1;
    bool cacheDisable : 1;
    bool accessed : 1;
    bool ignored0 : 1;
    bool pageSizeZero : 1; // This must be set to 0 (false)
    uint64_t ignored1 : 4;
    uint64_t pageDirectoryPhysAddr : 36; // Only the significant bits, exclude last 12 bits (since Page Directory physical address is 0x1000 byte aligned, hardware does not need last 12 bits)
    uint64_t zero : 4;
    uint64_t ignored2 : 11;
    bool xd : 1;
} __attribute__((packed));

struct PDPT
{
    PDPTEntry entries[512];
} __attribute__((aligned(0x1000)));

struct PageDirectoryEntry
{
    bool present : 1;
    bool readWrite : 1;
    bool userSupervisor : 1;
    bool writeThrough : 1;
    bool cacheDisable : 1;
    bool accessed : 1;
    bool ignored0 : 1;
    bool pageSizeZero : 1; // This must be set to 0 (false)
    uint64_t ignored1 : 4;
    uint64_t pageTablePhysAddr : 36; // Only the significant bits, exclude last 12 bits (since Page Table physical address is 0x1000 byte aligned, hardware does not need last 12 bits)
    uint64_t zero : 4;
    uint64_t ignored2 : 11;
    bool xd : 1;
} __attribute__((packed));

struct PageDirectory
{
    PageDirectoryEntry entries[512];
} __attribute__((aligned(0x1000)));

struct PageTableEntry
{
    bool present : 1;
    bool readWrite : 1;
    bool userSupervisor : 1;
    bool writeThrough : 1;
    bool cacheDisable : 1;
    bool accessed : 1;
    bool dirty : 1;
    bool pat : 1;
    bool global : 1;
    uint64_t ignored0 : 3;
    uint64_t pageFramePhysAddr : 36; // Only the significant bits, exclude last 12 bits (since page frames are obviously 0x1000 byte aligned, hardware does not need last 12 bits)
    uint64_t zero : 4;
    uint64_t ignored1 : 7;
    uint64_t protectionKey : 4;
    bool xd : 1;
} __attribute__((packed));

struct PageTable
{
    PageTableEntry entries[512];
} __attribute__((aligned(0x1000)));

#endif
