#include "DeviceFS.h"
#include "Device.h"
#include "TerminalDevice.h"
#include "FramebufferDevice.h"
#include "KeyboardDevice.h"
#include "Serial.h"
#include "Heap.h"

DeviceFS::DeviceFS(Disk* disk) : FileSystem(disk)
{
    fileSystemRoot = new (Allocator::Permanent) VFS::Vnode();
    fileSystemRoot->inodeNum = currentInodeNum++;
    fileSystemRoot->type = VFS::VnodeType::Directory;
    fileSystemRoot->fileSystem = this;
    VFS::CacheVNode(fileSystemRoot);

    Device* terminal = new TerminalDevice(String("tty"), currentInodeNum++);
    devices.Push(terminal);

    auto terminalVnode = new (Allocator::Permanent) VFS::Vnode();
    terminalVnode->inodeNum = terminal->GetInodeNumber();
    terminalVnode->fileSystem = this;
    terminalVnode->context = terminal;
    terminalVnode->fileSize = 0;
    terminalVnode->type = VFS::VnodeType::Terminal;
    VFS::CacheVNode(terminalVnode);

    Device* framebuffer = new FramebufferDevice(String("fb"), currentInodeNum++);
    devices.Push(framebuffer);

    auto framebufferVnode = new (Allocator::Permanent) VFS::Vnode();
    framebufferVnode->inodeNum = framebuffer->GetInodeNumber();
    framebufferVnode->fileSystem = this;
    framebufferVnode->context = framebuffer;
    framebufferVnode->fileSize = 0;
    framebufferVnode->type = VFS::VnodeType::Framebuffer;
    VFS::CacheVNode(framebufferVnode);

    Device* keyboard = new KeyboardDevice(String("kbd"), currentInodeNum++);
    devices.Push(keyboard);

    auto keyboardVnode = new (Allocator::Permanent) VFS::Vnode();
    keyboardVnode->inodeNum = keyboard->GetInodeNumber();
    keyboardVnode->fileSystem = this;
    keyboardVnode->context = keyboard;
    keyboardVnode->fileSize = 0;
    keyboardVnode->type = VFS::VnodeType::Keyboard;
    VFS::CacheVNode(keyboardVnode);
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

void DeviceFS::Create(VFS::Vnode* vnode, VFS::Vnode* directory, const String& name)
{
    Panic();

    (void)vnode;
    (void)directory;
    (void)name;
}

void DeviceFS::Truncate(VFS::Vnode* vnode)
{
    Panic();

    (void)vnode;
}
