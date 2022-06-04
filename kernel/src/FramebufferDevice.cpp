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
    Assert(count % 4 == 0);
    Assert(position % 4 == 0);
    memcpy(Framebuffer::GetBuffer() + (position / 4), buffer, count);
    return count;
}

FramebufferDevice::FramebufferDevice(const String& name, uint32_t inodeNum) : Device(name, inodeNum) {}
