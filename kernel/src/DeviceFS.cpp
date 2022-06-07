#include "DeviceFS.h"
#include "Device.h"
#include "TerminalDevice.h"
#include "FramebufferDevice.h"
#include "KeyboardDevice.h"
#include "RandomDevice.h"
#include "Serial.h"
#include "Heap.h"

DeviceFS::DeviceFS(Disk* disk) : FileSystem(disk)
{
    fileSystemRoot = VFS::ConstructVnode(currentInodeNum++, this, nullptr, 0, VFS::VnodeType::Directory);

    Device* terminal = new TerminalDevice(String("tty"), currentInodeNum++);
    devices.Push(terminal);
    VFS::ConstructVnode(terminal->GetInodeNumber(), this, terminal, 0, VFS::VnodeType::Terminal);

    Device* framebuffer = new FramebufferDevice(String("fb"), currentInodeNum++);
    devices.Push(framebuffer);
    VFS::ConstructVnode(framebuffer->GetInodeNumber(), this, framebuffer, 0, VFS::VnodeType::Framebuffer);

    Device* keyboard = new KeyboardDevice(String("kbd"), currentInodeNum++);
    devices.Push(keyboard);
    VFS::ConstructVnode(keyboard->GetInodeNumber(), this, keyboard, 0, VFS::VnodeType::Keyboard);

    Device* random = new RandomDevice(String("urandom"), currentInodeNum++);
    devices.Push(random);
    VFS::ConstructVnode(random->GetInodeNumber(), this, random, 0, VFS::VnodeType::Random);
}

uint64_t DeviceFS::Read(VFS::Vnode* vnode, void* buffer, uint64_t count, uint64_t readPos)
{
    auto device = reinterpret_cast<Device*>(vnode->context);
    return device->Read(buffer, count, readPos);
}

uint64_t DeviceFS::Write(VFS::Vnode* vnode, const void* buffer, uint64_t count, uint64_t writePos)
{
    auto device = reinterpret_cast<Device*>(vnode->context);
    return device->Write(buffer, count, writePos);
}

VFS::Vnode* DeviceFS::FindInDirectory(VFS::Vnode* directory, const String& name)
{
    Assert(directory == fileSystemRoot);

    for (Device* device : devices)
    {
        Serial::Log("[/dev]------------- Found: %s", device->GetName().ToRawString());

        if (device->GetName().Equals(name))
        {
            VFS::Vnode* file = VFS::SearchInCache(device->GetInodeNumber(), this);

            Assert(file != nullptr);

            return file;
        }
    }

    return nullptr;
}

VFS::DirectoryEntry DeviceFS::ReadDirectory(VFS::Vnode* directory, uint64_t readPos)
{
    Panic();

    (void)directory;
    (void)readPos;
}

String DeviceFS::GetPathFromSymbolicLink(VFS::Vnode* symLinkVnode)
{
    Panic();
    (void)symLinkVnode;
}

VFS::Vnode* DeviceFS::Create(VFS::Vnode* directory, const String& name, VFS::VnodeType vnodeType)
{
    Panic();
    return nullptr;

    (void)directory;
    (void)name;
    (void)vnodeType;
}

void DeviceFS::Truncate(VFS::Vnode* vnode)
{
    Panic();

    (void)vnode;
}
