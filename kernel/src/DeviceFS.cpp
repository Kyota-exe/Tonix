#include "DeviceFS.h"
#include "Device.h"
#include "Terminal.h"
#include "Serial.h"

void DeviceFS::Mount(Vnode* mountPoint)
{
    devRoot = new Vnode();
    devRoot->inodeNum = currentInodeNum++;
    devRoot->type = VFSDirectory;
    devRoot->fileSystem = this;

    mountPoint->mountedVNode = devRoot;
    CacheVNode(devRoot);

    // Create terminal
    Device* terminal = new Terminal(String("tty"), currentInodeNum++);
    devices.Push(terminal);

    auto terminalVnode = new Vnode();
    terminalVnode->inodeNum = terminal->inodeNum;
    terminalVnode->fileSystem = this;
    terminalVnode->context = terminal;
    terminalVnode->fileSize = 0;
    terminalVnode->type = VFSCharacterDevice;
    CacheVNode(terminalVnode);
}

uint64_t DeviceFS::Read(Vnode* vnode, void* buffer, uint64_t count, uint64_t readPos)
{
    auto device = (Device*)vnode->context;
    return device->Read(buffer, count);

    (void)readPos;
}

uint64_t DeviceFS::Write(Vnode* vnode, const void* buffer, uint64_t count, uint64_t writePos)
{
    auto device = (Device*)vnode->context;
    return device->Write(buffer, count);

    (void)writePos;
}

Vnode* DeviceFS::FindInDirectory(Vnode* directory, const String& name)
{
    KAssert(directory == devRoot, "[/dev] Directory is not /dev.");

    for (Device* device : devices)
    {
        Serial::Print("[/dev]------------- Found: ", "");
        Serial::Print(device->name->ToCString());

        if (device->name->Equals(name))
        {
            Vnode* file = SearchInCache(device->inodeNum, this);

            KAssert(file != nullptr, "Device file not found.");

            return file;
        }
    }

    return nullptr;
}

void DeviceFS::Create(Vnode* vnode, Vnode* directory, const String& name)
{
    Panic("Cannot create device file.");

    (void)vnode;
    (void)directory;
    (void)name;
}

void DeviceFS::Truncate(Vnode* vnode)
{
    Panic("Cannot truncate device file.");

    (void)vnode;
}