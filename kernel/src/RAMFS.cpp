#include "RAMFS.h"

/*struct File
{
    char* fileName;
    uint64_t fileSize;
    bool valid = false;
    bool directory = false;
};

Vector<FileInode> files;
Vector<DirectoryInode> directories;

void InitializeRAMFS(uint64_t ramDiskBegin, uint64_t ramDiskEnd)
{
    Serial::Print("Initializing USTAR file system in RAM disk...");
    Serial::Printf("RAM disk size: %x", ramDiskEnd - ramDiskBegin);

    // Parse ALL of the files in the .tar RAM disk
    int parsedFilesCount = 0;
    uint64_t tarReadPos = ramDiskBegin;

    // Read root directory
    DirectoryInode currentDirectory;


    while (tarReadPos < ramDiskEnd)
    {
        uint8_t* header = (uint8_t*)tarReadPos;
        if (*header == 0) break;

        char* completeFileName = NULL;
        // File name
        {
            char* fileName = (char*)header;
            char* fileNamePrefix = (char*)&header[345];

            uint64_t fileNameLength = StringLength(fileName, 100);
            uint64_t fileNamePrefixLength = StringLength(fileNamePrefix, 155);

            completeFileName = (char*)KMalloc(fileNameLength + fileNamePrefixLength + 1);
            StringCopy(fileNamePrefix, completeFileName, fileNamePrefixLength);
            StringCopy(fileName, completeFileName, fileNameLength, fileNamePrefixLength);

            completeFileName[fileNameLength + fileNamePrefixLength] = 0;
        }

        bool isDirectory = (char*)&header[156] == '5';
        if (!isDirectory)
        {
            FileInode fileInode;

            // File size
            // The size of this field is 12 bytes
            // We use an array of 13 characters to manually add the NULL terminator in case it is not there.
            char fileSizeString[13] {};
            StringCopy((char*)&header[124], fileSizeString, 12);
            fileSizeString[12] = 0;
            fileInode.fileSize = StringOctalToUInt(fileSizeString);
        }
        else
        {
            DirectoryInode directoryInode;

        }

        Serial::Printf("Size: %dB", nextFile->fileSize);

        Serial::Print("Name: ", "");
        Serial::Print(nextFile->fileName, "\n\n");

        tarReadPos += 512;
        tarReadPos += ((nextFile->fileSize / 512) + (nextFile->fileSize % 512 != 0 ? 1 : 0)) * 512;

        nextFile->valid = true;
        nextFile++;
        parsedFilesCount++;
    }
    Serial::Printf("Parsed %d files, including directories.", parsedFilesCount);
}

DirectoryInode RequestDirectoryListing()
{
    DirectoryInode dirInode = KMalloc(sizeof(DirectoryInode));

    Vector<TNode> fileTNodes = KMalloc(sizeof(Vector<TNode>));
    Vector<TNode> subdirectoryTNodes = KMalloc(sizeof(Vector<TNode>));

    for (uint64_t i = 0; i < 32; ++i)
    {

    }
}*/