#include "FramebufferDevice.h"
#include "Framebuffer.h"
#include "Memory/Memory.h"

uint64_t FramebufferDevice::Read(void* buffer, uint64_t count, uint64_t position)
{
    Panic();
    (void)buffer;
    (void)count;
    (void)position;
}

uint64_t FramebufferDevice::Write(const void* buffer, uint64_t count, uint64_t position)
{
    Assert(position == 0);
    Assert(count == sizeof(uint32_t) * Framebuffer::Height() * Framebuffer::Width());
    memcpy(Framebuffer::GetBuffer(), buffer, count);
    return count;
}

FramebufferDevice::FramebufferDevice(const String& name, uint32_t inodeNum) : Device(name, inodeNum) {}