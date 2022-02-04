#include "FileSystem.h"

FileSystem::FileSystem(Disk* disk) : disk(disk) { }

FileSystem::~FileSystem()
{
    delete disk;
}