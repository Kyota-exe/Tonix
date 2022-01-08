#include "RAMFS.h"

struct File
{
    char* fileName;
    uint64_t fileSize;
    bool valid = false;
};

File files[32];
File* nextFile;

void InitializeRAMFS(uint64_t ramDiskBegin, uint64_t ramDiskEnd)
{
    Serial::Printf("Ram disk size: %x", ramDiskEnd - ramDiskBegin);

    int parsedFilesCount = 0;
    nextFile = files;
    uint64_t tarReadPos = ramDiskBegin;
    while (tarReadPos < ramDiskEnd)
    {
        uint8_t* header = (uint8_t*)tarReadPos;
        if (*header == 0) break;

        // File size
        // The size of this field is 12 bytes
        // We use an array of 13 characters to manually add the NULL terminator in case it is not there.
        char fileSizeString[13] {};
        StringCopy((char*)&header[124], fileSizeString, 12);
        fileSizeString[12] = 0;
        nextFile->fileSize = StringOctalToUInt(fileSizeString);

        // File name
        {
            char* fileName = (char*)header;
            char* fileNamePrefix = (char*)&header[345];

            uint64_t fileNameLength = StringLength(fileName, 100);
            uint64_t fileNamePrefixLength = StringLength(fileNamePrefix, 155);

            nextFile->fileName = (char*)KMalloc(fileNameLength + fileNamePrefixLength + 1);
            StringCopy(fileNamePrefix, nextFile->fileName, fileNamePrefixLength);
            StringCopy(fileName, nextFile->fileName, fileNameLength, fileNamePrefixLength);

            nextFile->fileName[fileNameLength + fileNamePrefixLength] = 0;
        }

        Serial::Print("Type: ", "");
        Serial::Print((char*)&header[156]);

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