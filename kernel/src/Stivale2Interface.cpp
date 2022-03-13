#include "Stivale2Interface.h"

// Uninitialized static (stored in .bss) uint8_t array will act as stack to pass to stivale2
static uint8_t stack[8192];

stivale2_struct* stivale2Struct;

void* GetStivale2Tag(uint64_t id)
{
    // Loop through each tag supplied by stivale2 and compare its identifier to the identifier of the tag we want.
    auto currentTag = reinterpret_cast<stivale2_tag*>(stivale2Struct->tags);
    while (currentTag != nullptr)
    {
        if (currentTag->identifier == id)
        {
            return currentTag;
        }

        currentTag = reinterpret_cast<stivale2_tag*>(currentTag->next);
    }

    return nullptr;
}

void InitializeStivale2Interface(stivale2_struct* _stivale2Struct)
{
    stivale2Struct = _stivale2Struct;
}

// Last node of linked list of stivale2 tags.
// The next node added will be placed before this one and so on
// until the last node added will act as the head of the linked list.
// This node requests access to the framebuffer supplied by stivale2.
static stivale2_header_tag_framebuffer framebufferHeaderTag
{
    .tag
    {
        .identifier = STIVALE2_HEADER_TAG_FRAMEBUFFER_ID,
        .next = 0
    },
    // Setting these to 0 tells the bootloader it can pick the best it can.
    .framebuffer_width  = 0,
    .framebuffer_height = 0,
    .framebuffer_bpp = 0,

    .unused = 0
};

static stivale2_header_tag_smp smpHeaderTag
{
    .tag
    {
        .identifier = STIVALE2_HEADER_TAG_SMP_ID,
        .next = (uint64_t)&framebufferHeaderTag
    },
    .flags = 0
};

__attribute__((section(".stivale2hdr"), used))
static stivale2_header stivale2Header
{
    // Settings this to 0 makes it so that the entry point specified in the ELF file is used.
    .entry_point = 0,

    // The stack grows downwards.
    // .stack is the base pointer of the stack, so we must add sizeof(stack) to it.
    .stack = reinterpret_cast<uintptr_t>(stack + sizeof(stack)),

    // Bit 0: Reserved, formerly used to indicate whether to enable KASLR.
    // Bit 1: If this is set, virtual addresses in the higher half will point to physical addresses starting from 0.
    // Bit 2: If this is set, protected memory ranges will be enabled. This means that stivale2 will map the kernel according to its ELF segments.
    // Bit 3: If this is set, fully virtual kernel mappings for PMRs will be enabled. Only works if PMRs are enabled.
    // Bit 4: This bit should always be set as it is related to deprecated functionality.
    .flags = (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4),

    // Points to head of linked list of tags, which is the last node we added.
    .tags = reinterpret_cast<uintptr_t>(&smpHeaderTag)
};