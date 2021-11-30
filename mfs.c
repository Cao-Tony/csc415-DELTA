/**************************************************************
* Class:  CSC-415 
* Name: Tony Cao, Dominique Dutton, Kandace Bishop
* Student ID: 920171613 (Tony), 920820781 (Dominique), 918762889 (Kandace)
* GitHub Handle:  Cao-Tony, kbishop1-sfsu
* Project: File System
*
* File: mfs.c
*
* Description: file system interface needed by the driver to 
* interact with your filesystem.
**************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "fsLow.h"
#include "mfs.h"
#include "volumeControlBlock.h"
#include "freeSpace.h"

char currentPath[500];
char currentDirName[20];
fdDir *currentDir;
struct fs_stat currentDirStats;
fdDir *tempDir;              
struct fs_stat tempDirStats; 
struct fs_diriteminfo *CurrentDirItemInfo = NULL;
uint64_t openDir = 0;
int currentEntry = 0;

int fileQueIsReady = 0; 
fileQue fileQueArray[MAXFILEQUE];

int instantiateFileQue();
int closeFileQue();
int fs_createFile(char *, int);

void printCurrentDir();
void printFsStats();

int fs_WriteDir(fdDir *);
struct fs_stat fs_readDirStats(uint64_t dirLoc);
int fs_readDirFromVol(fdDir *dir, uint64_t dirLoc);
int fs_setCurrentDirectory(uint64_t);
int fs_closeCurrentDir();
void fs_writeStats(struct fs_stat, uint64_t);
int recDelDir(uint64_t dirLoc);
int openTempDir(uint64_t dirLoc);
int fs_closeDirWOWrite(fdDir *);
int recurvefs_delete(char *filename);

int fs_loadRoot()
{
    uint64_t rootDirLoc = getRootDirectory();
    memcpy(currentPath, "/", 1);
    memcpy(currentDirName, "/", 1);
    return fs_setCurrentDirectory(rootDirLoc);
}

int fs_closeCurrentDir()
{
    fs_writeStats(currentDirStats, currentDir[0].directoryStartLocation);
    return fs_closedir(currentDir);
}

int mfs_shutdown()
{
    closeFileQue();
    fs_closeCurrentDir();
    return 0;
}

void printCurrentDir()
{
    printf("Printing current dir\n");
    int numberOfFileEntries = getSOB() / sizeof(struct fdDir);
    for (int i = 0; i < numberOfFileEntries; i++)
    {
        printf("%s\t", currentDir[i].name);                    
        printf("%d\t", currentDir[i].fileType);                
        printf("%d\t", currentDir[i].size);                    
        printf("%ld\t", currentDir[i].directoryStartLocation); 
        printf("%d\t", currentDir[i].d_reclen);                
        printf("%d\n", currentDir[i].dirEntryPosition);        
    }
}

void printFsStats()
{
    printf("Printing fs_stats\n");
    printf("%li\t", currentDirStats.st_size);
    printf("%li\t", currentDirStats.st_blksize);
    printf("%li\t", currentDirStats.st_blocks);
    printf("%li\t", currentDirStats.st_accesstime);
    printf("%li\t", currentDirStats.st_modtime);
    printf("%li\n", currentDirStats.st_createtime);

    for (int i = 0; i < NUMBEROFENTRIESALLOWED; i++)
    {
        if (currentDirStats.EntryArray[i] != 0)
        {
            printf("%li\t", currentDirStats.EntryArray[i]);
        }
    }
    printf("\n");
}

int fs_mkdir(const char *pathName, mode_t mode)
{

    int numberOfFileEntries = getSOB() / sizeof(struct fdDir); 
    int openDirectoryEntry;

    if (currentDir != NULL)
    {
        int i = 0;
        for (; i < numberOfFileEntries; i++)
        {
            if (strcmp(pathName, currentDir[i].name) == 0)
            {
                printf("Name already in use, try again.\n");
                return 0;
            }
            if (currentDir[i].fileType == 0)
            {
                openDirectoryEntry = i;
                break;
            }
        }
        if (i == numberOfFileEntries - 1)
        {
            printf("No more space available in current Directory...\n");
            return 0;
        }
    }

    /****************************************************
	* Start creating Directory fs_stats
	****************************************************/

    int whereFSStatsIs = allocateFreeSpace();
    struct fs_stat tempstats;
    time_t currentTime = time(NULL);
    tempstats.st_blksize = getSOB();
    tempstats.st_blocks = 2; 
    tempstats.st_size = tempstats.st_blksize * tempstats.st_blocks;
    tempstats.st_accesstime = currentTime;
    tempstats.st_modtime = currentTime;
    tempstats.st_createtime = currentTime;
    tempstats.EntryArray[0] = allocateFreeSpace(); 
    for (int i = 1; i < NUMBEROFENTRIESALLOWED; i++)
    {
        tempstats.EntryArray[i] = 0;
    }

    /****************************************************
	* Start creating Directory Entries
	****************************************************/

    fdDir *DE = malloc(numberOfFileEntries * sizeof(struct fdDir));

    strcpy(DE[0].name, ".");
    strcpy(DE[1].name, "..");
    DE[0].directoryStartLocation = whereFSStatsIs; 
    DE[0].fileType = DIR;
    DE[0].size = 2; 
    DE[0].d_reclen = numberOfFileEntries;
    DE[0].dirEntryPosition = tempstats.EntryArray[0]; 
    if (strcmp(pathName, "root") == 0)
    {
        DE[1].directoryStartLocation = DE[0].directoryStartLocation;
        DE[1].fileType = DE[0].fileType;
        DE[1].size = DE[0].size;
        DE[1].d_reclen = DE[0].d_reclen;
        DE[1].dirEntryPosition = DE[0].dirEntryPosition;
    }
    else
    {
        strcpy(currentDir[openDirectoryEntry].name, pathName);
        currentDir[openDirectoryEntry].directoryStartLocation = DE[0].directoryStartLocation;
        currentDir[openDirectoryEntry].fileType = DIR;
        currentDir[openDirectoryEntry].size = DE[0].size;
        currentDir[openDirectoryEntry].d_reclen = DE[0].d_reclen;
        currentDir[openDirectoryEntry].dirEntryPosition = DE[0].dirEntryPosition;

        DE[1].directoryStartLocation = currentDir[0].directoryStartLocation;
        DE[1].fileType = currentDir[0].fileType;
        DE[1].size = currentDir[0].size;
        DE[1].d_reclen = currentDir[0].d_reclen;
        DE[1].dirEntryPosition = currentDir[0].dirEntryPosition;

        /****************************************************
         * Increasing size needs to be fixed
        ***************************************************/
        currentDirStats.st_size += getSOB() * 2;
        currentDirStats.st_modtime = time(NULL);
    }

    for (int i = 2; i < numberOfFileEntries; i++) 
    {
        strcpy(DE[i].name, ".EMPTY");
        DE[i].fileType = FREE;
        DE[i].size = 0;
        DE[i].directoryStartLocation = 0;
        DE[i].d_reclen = 0;
        DE[i].dirEntryPosition = 0;
    }

    fs_writeStats(tempstats, DE[0].directoryStartLocation);
    fs_closedir(DE);
    if (strcmp(pathName, "root") != 0)
    {
        uint64_t reopen = currentDir[0].directoryStartLocation;
        fs_closeCurrentDir();
        fs_setCurrentDirectory(reopen);
    }

    return 0;
}

void fs_writeStats(struct fs_stat fs_statToWrite, uint64_t lbaToRightStatsTo)
{
    //printf("wrote stats to %ld\n", lbaToRightStatsTo);
    char *tempBuf = malloc(getSOB());
    char *tempSecondayBuf = malloc(100);

    fs_statToWrite.st_accesstime = time(NULL);

    sprintf(tempBuf, "%li`%li`%li`%li`%li`%li`",
            fs_statToWrite.st_size,
            fs_statToWrite.st_blksize,
            fs_statToWrite.st_blocks,
            fs_statToWrite.st_accesstime,
            fs_statToWrite.st_modtime,
            fs_statToWrite.st_createtime);

    for (int i = 0; i < NUMBEROFENTRIESALLOWED; i++)
    {
        sprintf(tempSecondayBuf, "%li`", fs_statToWrite.EntryArray[i]);
        strcat(tempBuf, tempSecondayBuf);
    }

    LBAwrite(tempBuf, 1, lbaToRightStatsTo);
    free(tempBuf);
    tempBuf = NULL;
    free(tempSecondayBuf);
    tempSecondayBuf = NULL;
    return;
}

int recDelDir(uint64_t dirLoc) 
{
    // printf("Calling recDelDir\n");
    int filesDeleted = 0;
    int numberOfFileEntries = getSOB() / sizeof(struct fdDir);
    int openDirErrorCode = openTempDir(dirLoc); 
    fdDir *dirToBeDeleted = malloc(numberOfFileEntries * sizeof(struct fdDir));
    for (int j = 0; j < numberOfFileEntries; j++)
    {
        strcpy(dirToBeDeleted[j].name, tempDir[j].name);
        dirToBeDeleted[j].fileType = tempDir[j].fileType;
        dirToBeDeleted[j].size = tempDir[j].size;
        dirToBeDeleted[j].directoryStartLocation = tempDir[j].directoryStartLocation;
        dirToBeDeleted[j].d_reclen = tempDir[j].d_reclen;
        dirToBeDeleted[j].dirEntryPosition = tempDir[j].dirEntryPosition;
    }

    struct fs_stat statsToBeDeleted = tempDirStats;
    statsToBeDeleted.st_size = tempDirStats.st_size;
    statsToBeDeleted.st_blksize = tempDirStats.st_blksize;
    statsToBeDeleted.st_blocks = tempDirStats.st_blocks;
    statsToBeDeleted.st_blocks = tempDirStats.st_blocks;
    statsToBeDeleted.st_accesstime = tempDirStats.st_accesstime;
    statsToBeDeleted.st_createtime = tempDirStats.st_createtime;
    for (int i = 0; i < NUMBEROFENTRIESALLOWED; i++)
    {
        statsToBeDeleted.EntryArray[i] = tempDirStats.EntryArray[i];
    }

    fs_closeDirWOWrite(tempDir);

    for (int i = 2; i < numberOfFileEntries; i++)
    {
        if (dirToBeDeleted[i].fileType == DIR)
        {
            filesDeleted += recDelDir(dirToBeDeleted[i].directoryStartLocation);
            filesDeleted += 1;
        }
        else if (dirToBeDeleted[i].fileType == FILE)
        {
            openTempDir(dirLoc);
            recurvefs_delete(dirToBeDeleted[i].name);
            fs_closedir(tempDir);
            filesDeleted += 1;
        }
    }
    dirToBeDeleted[0].fileType = FREE;
    if (releaseFreeSpace(dirToBeDeleted[0].directoryStartLocation) == 0)
        dirToBeDeleted[0].directoryStartLocation = 0;
    if (releaseFreeSpace(dirToBeDeleted[0].dirEntryPosition) == 0)
        dirToBeDeleted[0].dirEntryPosition = 0;

    fs_closedir(dirToBeDeleted);
    return filesDeleted;
}

int fs_rmdir(const char *pathname)
{
    if (strcmp(pathname, ".") == 0 || strcmp(pathname, "..") == 0)
    {
        printf("Can not remove parent or self\n");
        return 0;
    }

    int numberOfFileEntries = getSOB() / sizeof(struct fdDir);
    int neededDirectoryEntryToDelete = 0;
    int numberOfItemsDeleted = 0;
    for (int i = 2; i < numberOfFileEntries; i++)
    {
        if (strcmp(pathname, currentDir[i].name) == 0)
        {
            neededDirectoryEntryToDelete = i;
            numberOfItemsDeleted = recDelDir(currentDir[i].directoryStartLocation);
            break;
        }
    }

    numberOfItemsDeleted += 1;

    unsigned int sizeToReduceBy;
    fs_setCurrentDirectory(currentDir[neededDirectoryEntryToDelete].directoryStartLocation);
    sizeToReduceBy = currentDirStats.st_size;
    fs_setCurrentDirectory(currentDir[1].directoryStartLocation);

    /****************************************************
     * Reducing size needs to be fixed
     ***************************************************/
    currentDirStats.st_modtime = time(NULL);

    // clear entry in current directory
    strcpy(currentDir[neededDirectoryEntryToDelete].name, ".EMPTY");
    currentDir[neededDirectoryEntryToDelete].fileType = FREE;
    if (releaseFreeSpace(currentDir[neededDirectoryEntryToDelete].directoryStartLocation))
        currentDir[neededDirectoryEntryToDelete].directoryStartLocation = 0; // free fs_stats
    if (releaseFreeSpace(currentDir[neededDirectoryEntryToDelete].dirEntryPosition))
        currentDir[neededDirectoryEntryToDelete].dirEntryPosition = 0; // free DE block

    uint64_t reopen = currentDir[0].directoryStartLocation;
    fs_closeCurrentDir();
    fs_setCurrentDirectory(reopen);

    return 0;
}

fdDir *fs_opendir(const char *name)
{
    int numberOfFileEntries = getSOB() / sizeof(struct fdDir);
    fdDir *DE = malloc(numberOfFileEntries * sizeof(struct fdDir));
    if (strcmp(name, currentPath) == 0 || strcmp(name, ".") == 0)
    {
        for (int i = 0; i < numberOfFileEntries; i++) //Initialize all file entries
        {
            strcpy(DE[i].name, currentDir[i].name);
            DE[i].fileType = currentDir[i].fileType;
            DE[i].size = currentDir[i].size;
            DE[i].directoryStartLocation = currentDir[i].directoryStartLocation;
            DE[i].d_reclen = currentDir[i].d_reclen;
            DE[i].dirEntryPosition = currentDir[i].dirEntryPosition;
        }
        if (openDir == 1)
        {
            // return to Parent
            fs_setCurrentDirectory(currentDir[1].directoryStartLocation);
            openDir = 0;
        }
        else if (openDir > 1)
        {
            // return to Child
            fs_setCurrentDirectory(openDir);
            openDir = 0;
        }
        return DE;
    }
    for (int i = 1; i < numberOfFileEntries; i++)
    {
        if (strcmp(name, currentDir[i].name) == 0 && currentDir[i].fileType == DIR)
        {
            // Enter Directory
            printf("%s directory Contains:\n", name);
            if (strcmp(name, "..") == 0)
                openDir = currentDir[0].directoryStartLocation;
            else
                openDir = 1;

            uint64_t returnTo = currentDir[i].directoryStartLocation;
            fs_closeCurrentDir();
            fs_setCurrentDirectory(returnTo);
            return fs_opendir(currentDir[0].name);
        }
    }
    return NULL;
}

struct fs_diriteminfo *fs_readdir(fdDir *dirp)
{
    int numberOfFileEntries = getSOB() / sizeof(struct fdDir);

    if (CurrentDirItemInfo == NULL)
    {
        CurrentDirItemInfo = malloc(sizeof(struct fs_diriteminfo));
    }

    while (dirp[currentEntry].fileType == FREE &&
           currentEntry < numberOfFileEntries)
        currentEntry++;
    if (currentEntry == numberOfFileEntries)
    {
        free(CurrentDirItemInfo);
        CurrentDirItemInfo = NULL;
        currentEntry = 0;
        return NULL;
    }

    CurrentDirItemInfo->d_reclen = dirp[currentEntry].d_reclen;
    CurrentDirItemInfo->fileType = dirp[currentEntry].fileType;
    strcpy(CurrentDirItemInfo->d_name, dirp[currentEntry].name);
    currentEntry++;

    return CurrentDirItemInfo;
}

int fs_closedir(fdDir *dirp)
{
    fs_WriteDir(dirp);
    if (CurrentDirItemInfo != NULL)
    {
        free(CurrentDirItemInfo);
        CurrentDirItemInfo = NULL;
    }
    currentEntry = 0;
    free(dirp);
    dirp = NULL;
    return 0;
}

int fs_closeDirWOWrite(fdDir *dirp)
{
    free(dirp);
    dirp = NULL;
    return 0;
}

char *fs_getcwd(char *buf, size_t size) 
{
    if (strlen(buf) > size)
        return NULL;
    memcpy(buf, currentPath, strlen(currentPath));
    return currentPath;
}

int fs_setcwd(char *buf)
{
    if (strcmp(buf, ".") == 0)
        return 0;
    int numberOfFileEntries = getSOB() / sizeof(struct fdDir);
    for (int i = 0; i < numberOfFileEntries; i++)
    {
        if (currentDir[i].fileType == DIR && strcmp(currentDir[i].name, buf) == 0)
        {
            printf("Changing Directory...\n");
            if (strcmp(buf, "..") == 0)
            {
                // shrink currentPath
                int newLength = (strlen(currentPath) - strlen(currentDirName) - 1);
                if (newLength == 0)
                    newLength++;
                memcpy(currentPath + newLength, "\0", 1);
                for (int j = newLength + 1; j < 500; j++)
                    memcpy(currentPath + j, "", 1);
            }
            else
            {
                // expand working path
                memcpy(currentDirName, buf, strlen(buf));
                memcpy(currentDirName + strlen(buf), "\0", 1);
                if (strcmp(currentPath, "/") != 0)
                    memcpy(currentPath + strlen(currentPath), "/", 1);
                memcpy(currentPath + strlen(currentPath), buf, strlen(buf));
                memcpy(currentPath + strlen(currentPath), "\0", 1);
            }
            uint64_t returnTo = currentDir[i].directoryStartLocation;
            fs_closeCurrentDir();
            return fs_setCurrentDirectory(returnTo);
        }
    }
    return 1;
}

int fs_isFile(char *path) 
{
    int numberOfFileEntries = getSOB() / sizeof(struct fdDir);
    for (int i = 0; i < numberOfFileEntries; i++)
    {
        if (strcmp(path, currentDir[i].name) == 0 &&
            currentDir[i].fileType == FILE)
            return 1;
    }
    return 0;
}

int fs_isDir(char *path) 
{
    int numberOfFileEntries = getSOB() / sizeof(struct fdDir);
    for (int i = 0; i < numberOfFileEntries; i++)
    {
        if (strcmp(path, currentDir[i].name) == 0 &&
            currentDir[i].fileType == DIR)
            return 1;
    }
    return 0;
}

int fs_delete(char *filename) //removes a file
{
    // printf("Calling fs_delet\n");
    int numberOfFileEntries = getSOB() / sizeof(struct fdDir);
    for (int i = 2; i < numberOfFileEntries; i++)
    {
        if (strcmp(filename, currentDir[i].name) == 0)
        {
            // printf("Current Dir: %d\n", i);
            struct fs_stat statsToDelete = fs_readDirStats(currentDir[i].directoryStartLocation);
            for (int j = 0; j < NUMBEROFENTRIESALLOWED; j++)
            {
                if (statsToDelete.EntryArray[j] > 0)
                    releaseFreeSpace(statsToDelete.EntryArray[j]);
            }
            strcpy(currentDir[i].name, ".EMPTY");
            currentDir[i].fileType = 0;
            // printf("directoryStartLocation %ld\n", currentDir[i].directoryStartLocation);
            releaseFreeSpace(currentDir[i].directoryStartLocation);
            // printf("File removed\n");
            uint64_t returnTo = currentDir[0].directoryStartLocation;
            fs_closeCurrentDir();
            fs_setCurrentDirectory(returnTo);
            return 0;
        }
    }
    printf("Cannot find file to remove...\n");
    return 0;
}

int recurvefs_delete(char *filename)
{
    int numberOfFileEntries = getSOB() / sizeof(struct fdDir);
    for (int i = 2; i < numberOfFileEntries; i++)
    {
        if (strcmp(filename, tempDir[i].name) == 0)
        {
            struct fs_stat statsToDelete = fs_readDirStats(tempDir[i].directoryStartLocation);
            for (int j = 0; j < NUMBEROFENTRIESALLOWED; j++)
            {
                if (statsToDelete.EntryArray[j] > 0)
                    releaseFreeSpace(statsToDelete.EntryArray[j]);
            }

            strcpy(tempDir[i].name, ".EMPTY");
            tempDir[i].fileType = 0;
            releaseFreeSpace(tempDir[i].directoryStartLocation);
            printf("File removed...\n");
            return 0;
        }
    }
    printf("Cannot find file to remove...\n");
    return 0;
}

int fs_initRoot()
{
    fs_mkdir("root", 0);
    int blockSize = getSOB();
    int totalBlocks = getTBC();
    unsigned int numberOfBytesNeededForFSM = ceil((double)totalBlocks / 4);
    unsigned int numberOfBlocksNeededForFSM = ceil((double)numberOfBytesNeededForFSM / blockSize);
    unsigned int rootDirLoc = numberOfBlocksNeededForFSM + (unsigned)1;
    setRootDirectory(rootDirLoc);
    return 0;
}

int fs_WriteDir(fdDir *dirp)
{
    // printf("Calling fs_writeDir\n");
    char *tempBuf = malloc(getSOB());
    char *tempChar = malloc(50 * sizeof(char));
    int numEntries = getSOB() / sizeof(struct fdDir);
    int offset = 0;
    int length = 0;
    for (int i = 0; i < numEntries; i++) 
    {
        sprintf(tempChar, "%s`%d`%d`%ld`%d`%d`",
                dirp[i].name,
                dirp[i].fileType,
                dirp[i].size,
                dirp[i].directoryStartLocation,
                dirp[i].d_reclen,
                dirp[i].dirEntryPosition);
        length += getPlaceValue(dirp[i].fileType);
        length += getPlaceValue(dirp[i].size);
        length += getPlaceValue(dirp[i].directoryStartLocation);
        length += getPlaceValue(dirp[i].d_reclen);
        length += getPlaceValue(dirp[i].dirEntryPosition);
        length += strlen(dirp[i].name);
        length += 6
        memcpy(tempBuf + (offset), tempChar, length);
        offset += length;
        length = 0;
    }
    for (int i = offset; i < getSOB(); i++)
    {
        memcpy(tempBuf + i, "", 1);
    }

    LBAwrite(tempBuf, 1, dirp[0].dirEntryPosition);

    free(tempChar);
    tempChar = NULL;
    free(tempBuf);
    tempBuf = NULL;

    return 0;
}

int fs_stat(const char *path, struct fs_stat *buf)
{
    // printf("Calling fs_stat\n");
    int numberOfFileEntries = getSOB() / sizeof(struct fdDir);
    int entryOfInterst = 0;
    struct fs_stat statsToRead;

    for (; entryOfInterst < numberOfFileEntries; entryOfInterst++)
    {
        if (strcmp(path, currentDir[entryOfInterst].name) == 0)
        {
            statsToRead = fs_readDirStats(currentDir[entryOfInterst].directoryStartLocation);
            buf->st_size = statsToRead.st_size;
            buf->st_blksize = statsToRead.st_blksize;
            buf->st_blocks = statsToRead.st_blocks;
            buf->st_accesstime = statsToRead.st_accesstime;
            buf->st_modtime = statsToRead.st_modtime;
            buf->st_createtime = statsToRead.st_createtime;
        }
    }
    return 0;
}

struct fs_stat fs_readDirStats(uint64_t dirLoc)
{
    char tempChar;
    char tempString[50];
    char *tempBuffer = malloc(getSOB());
    int stringPosition = 0;
    struct fs_stat returnValue;
    /****************************************************
	* Set fs_stats/returnValue
	****************************************************/
    LBAread(tempBuffer, 1, dirLoc);
    int maxCount = 6  + NUMBEROFENTRIESALLOWED;
    for (int i = 0, j = 0, delimCounter = 0; delimCounter < maxCount; i++)
    {
        tempChar = tempBuffer[i];
        if (tempChar == '`')
        {
            for (int k = stringPosition; k < 50; k++)
            {
                memcpy(tempString + k, "", 1);
            }
            switch (delimCounter)
            {
            case 0:
                returnValue.st_size = (off_t)atoi(tempString);
                break;
            case 1:
                returnValue.st_blksize = (blksize_t)atoi(tempString);
                break;
            case 2:
                returnValue.st_blocks = (blkcnt_t)atoi(tempString);
                break;
            case 3:
                returnValue.st_accesstime = (time_t)atoi(tempString);
                break;
            case 4:
                returnValue.st_modtime = (time_t)atoi(tempString);
                break;
            case 5:
                returnValue.st_createtime = (time_t)atoi(tempString);
                break;
            default:
                returnValue.EntryArray[j++] = (uint64_t)atoi(tempString);
                break;
            }
            stringPosition = 0;
            delimCounter++;
        }
        else if (tempChar == (char)0)
        {
            // do nothing
        }
        else
        {
            tempString[stringPosition++] = tempChar;
        }
    }

    free(tempBuffer);
    tempBuffer = NULL;
    return returnValue;
}

int fs_readDirFromVol(fdDir *directory, uint64_t entryLocation)
{
    char tempChar;
    char tempString[50];
    char *tempBuffer = malloc(getSOB());
    int stringPosition = 0;
    int numberOfFileEntries = getSOB() / sizeof(struct fdDir);

    LBAread(tempBuffer, 1, entryLocation);

    int curDir = 0; 
    int entry = 0; 

    for (int i = 0, j, stringPosition = 0; i < getSOB(); i++)
    {
        if (curDir == numberOfFileEntries)
            break;

        tempChar = tempBuffer[i];
        if (tempChar == '`')
        {
            for (int k = stringPosition; k < 50; k++)
            {
                memcpy(tempString + k, "", 1);
            }
            switch (entry)
            {
            case 0:
                strcpy(directory[curDir].name, tempString);
                break;
            case 1:
                directory[curDir].fileType = atoi(tempString);
                break;
            case 2:
                directory[curDir].size = atoi(tempString);
                break;
            case 3:
                directory[curDir].directoryStartLocation = atoi(tempString);
                break;
            case 4:
                directory[curDir].d_reclen = atoi(tempString);
                break;
            case 5:
                directory[curDir].dirEntryPosition = atoi(tempString);
                break;
            default:
                break;
            }
            stringPosition = 0;
            entry++;
        }
        else if (tempChar == (char)0)
        {
            // do nothing
        }
        else
        {
            tempString[stringPosition++] = tempChar;
        }

        if (entry == 6)
        {
            curDir++;
            entry = 0;
        }
    }

    free(tempBuffer);
    tempBuffer = NULL;
    return 0;
}

int fs_setCurrentDirectory(uint64_t dirLoc) 
{
    currentDirStats = fs_readDirStats(dirLoc);
    currentDirStats.st_accesstime = time(NULL);
    int numberOfFileEntries = getSOB() / sizeof(struct fdDir);
    currentDir = malloc(numberOfFileEntries * sizeof(struct fdDir));

    fs_readDirFromVol(currentDir, currentDirStats.EntryArray[0]);
    return 0;
}

int openTempDir(uint64_t dirLoc) 
{
    tempDirStats = fs_readDirStats(dirLoc);
    int numberOfFileEntries = getSOB() / sizeof(struct fdDir);
    tempDir = malloc(numberOfFileEntries * sizeof(struct fdDir));

    fs_readDirFromVol(tempDir, tempDirStats.EntryArray[0]);
    return 0;
}

int instantiateFileQue()
{
    for (int i = 0; i < MAXFILEQUE; i++)
    {
        fileQueArray[i].ocupado = 1;
        fileQueArray[i].buff = malloc(getSOB());
    }
    fileQueIsReady = 1; 
    return 0;
}

int closeFileQue()
{
    if (fileQueIsReady == 0)
        return 0;
    for (int i = 0; i < MAXFILEQUE; i++)
    {
        if (fileQueArray[i].ocupado == 0)
            fs_closeFile(i);
        free(fileQueArray[i].buff);
        fileQueArray[i].buff = NULL;
    }
    return 0;
}

int fs_openFile(char *fileToOpen, int flag)
{
    if (fileQueIsReady == 0)
    {
        if (instantiateFileQue() == 1)
        {
            printf("Error\n");
            return -1;
        }
    }
    int entryToInsert = 0;
    int fileExist = 0;
    for (int i = 0; i < MAXFILEQUE; i++)
    {
        if (fileQueArray[i].ocupado == 0 && strcmp(fileToOpen, fileQueArray[i].fileDir.name) == 0)
        {
            printf("Error File (%s) is already open.\n",fileQueArray[i].fileDir.name);
            return 0;
        }
    }
    for (int i = 2; i < NUMBEROFENTRIESALLOWED; i++)
    {
        if (strcmp(fileToOpen, currentDir[i].name) == 0)
        {
            entryToInsert = i;
            fileExist = 1;
            break;
        }
    }
    if (fileExist == 0) 
    {
        if (flag != O_RDONLY || flag != O_WRONLY) 
        {
            return fs_createFile(fileToOpen, flag);
        }
        printf("Could not find file in this Directroy\n");
        return -1;
    }
    int fileQueEntry = -1;
    for (int i = 0; i < MAXFILEQUE; i++)
    {
        if (fileQueArray[i].ocupado == 1) 
        {
            fileQueEntry = i;
            break;
        }
    }

    if (fileQueEntry < 0)
    {
        printf("FileQue is already full. Current limit of %i files. Please close a file.\n", MAXFILEQUE);
        return -1; 
    }

    struct fs_stat tempStatToRead = fs_readDirStats(currentDir[entryToInsert].directoryStartLocation);
    fileQueArray[fileQueEntry].fileDir.d_reclen = currentDir[entryToInsert].d_reclen;
    fileQueArray[fileQueEntry].fileDir.directoryStartLocation = currentDir[entryToInsert].directoryStartLocation;
    fileQueArray[fileQueEntry].fileDir.dirEntryPosition = currentDir[entryToInsert].dirEntryPosition;
    fileQueArray[fileQueEntry].fileDir.fileType = currentDir[entryToInsert].fileType;
    strcpy(fileQueArray[fileQueEntry].fileDir.name, currentDir[entryToInsert].name);
    fileQueArray[fileQueEntry].fileDir.size = currentDir[entryToInsert].size;
    time_t currentTime = time(NULL);

    for (int j = 0; j < NUMBEROFENTRIESALLOWED; j++)
    {
        fileQueArray[fileQueEntry].fileStats.EntryArray[j] = tempStatToRead.EntryArray[j];
    }

    fileQueArray[fileQueEntry].fileStats.st_accesstime = currentTime;
    fileQueArray[fileQueEntry].fileStats.st_blksize = getSOB();
    fileQueArray[fileQueEntry].fileStats.st_blocks = 1; 
    fileQueArray[fileQueEntry].fileStats.st_modtime = currentTime;
    fileQueArray[fileQueEntry].fileStats.st_size = tempStatToRead.st_size; 
    fileQueArray[fileQueEntry].flag = flag;
    fileQueArray[fileQueEntry].parentLBA = currentDir[0].directoryStartLocation; 
    fileQueArray[fileQueEntry].buffIndex = 0;
    fileQueArray[fileQueEntry].currentLocationInFile = 0;
    fileQueArray[fileQueEntry].ocupado = 0;
    return fileQueEntry; //return fileDescriptor
}

int fs_createFile(char *name, int flag)
{
    int entryToInsert;
    for (int i = 2; i < NUMBEROFENTRIESALLOWED; i++)
    {
        if (currentDir[i].fileType == FREE)
        {
            strcpy(currentDir[i].name, name);
            currentDir[i].fileType = FILE;
            currentDir[i].directoryStartLocation = allocateFreeSpace();
            currentDir[i].dirEntryPosition = allocateFreeSpace();
            for (int j = 0; j < MAXFILEQUE; j++)
            {
                if (fileQueArray[j].ocupado == 1)
                {
                    entryToInsert = j;
                    break;
                }
            }

            time_t currentTime = time(NULL);
            fileQueArray[entryToInsert].parentLBA = currentDir[0].directoryStartLocation;
            fileQueArray[entryToInsert].fileDir.directoryStartLocation = currentDir[i].directoryStartLocation;
            fileQueArray[entryToInsert].fileDir.dirEntryPosition = currentDir[i].dirEntryPosition;
            fileQueArray[entryToInsert].fileDir.size = 0;
            strcpy(fileQueArray[entryToInsert].fileDir.name, name);
            fileQueArray[entryToInsert].fileDir.fileType = FILE;
            fileQueArray[entryToInsert].fileDir.d_reclen = 0;
            fileQueArray[entryToInsert].fileStats.st_blksize = getSOB();
            fileQueArray[entryToInsert].fileStats.st_accesstime = currentTime;
            fileQueArray[entryToInsert].fileStats.st_createtime = currentTime;
            fileQueArray[entryToInsert].fileStats.st_modtime = currentTime;
            fileQueArray[entryToInsert].fileStats.EntryArray[0] = fileQueArray[entryToInsert].fileDir.dirEntryPosition;
            fileQueArray[entryToInsert].flag = flag;
            fileQueArray[entryToInsert].ocupado = 0;
            fileQueArray[entryToInsert].currentLocationInFile = 0;
            fileQueArray[entryToInsert].fileStats.st_blocks = 2;
            fileQueArray[entryToInsert].buffIndex = 0;

            fs_writeStats(fileQueArray[entryToInsert].fileStats, fileQueArray[entryToInsert].fileDir.directoryStartLocation);

            uint64_t reopen = currentDir[0].directoryStartLocation;
            fs_closeCurrentDir();
            fs_setCurrentDirectory(reopen);
            return entryToInsert;
        }
    }
    printf("Directory is full. can not exceed %i files.\n", NUMBEROFENTRIESALLOWED);
    return -1; 
}

int fs_closeFile(int fd)
{
    if (fileQueArray[fd].flag != O_RDONLY)
    {
        openTempDir(fileQueArray[fd].parentLBA);
        if (fileQueArray[fd].buffIndex > 0)
        {
            int filler = fileQueArray[fd].buffIndex;
            while (filler < getSOB())
            {
                memcpy(fileQueArray[fd].buff + filler++, "", 1);
            }

            int LBAToWriteTo = (int) ceil((float)(fileQueArray[fd].currentLocationInFile / getSOB()));
            LBAwrite(fileQueArray[fd].buff, 1, fileQueArray[fd].fileStats.EntryArray[LBAToWriteTo]);
            fileQueArray[fd].fileStats.st_size += fileQueArray[fd].buffIndex;
            fileQueArray[fd].fileDir.d_reclen += fileQueArray[fd].buffIndex;
        }
        fs_writeStats(tempDirStats, fileQueArray[fd].parentLBA);
        struct fs_stat tempStatToWrite = fs_readDirStats(fileQueArray[fd].fileDir.directoryStartLocation);
        for (int i = 2; i < NUMBEROFENTRIESALLOWED; i++)
        {
            if (tempDir[i].directoryStartLocation == fileQueArray[fd].fileDir.directoryStartLocation)
            {
                tempDir[i].size = ceil(fileQueArray[fd].fileStats.st_size / getSOB());
                tempDir[i].d_reclen = fileQueArray[fd].fileDir.d_reclen;
                for (int j = 0; j < NUMBEROFENTRIESALLOWED; j++)
                {   
                    tempStatToWrite.EntryArray[j] = fileQueArray[fd].fileStats.EntryArray[j];
                }
                break;
            }
        }
        tempStatToWrite.st_size = fileQueArray[fd].fileStats.st_size;
        fs_writeStats(tempStatToWrite, fileQueArray[fd].fileDir.directoryStartLocation);
        fs_closedir(tempDir);
    }
    fileQueArray[fd].currentLocationInFile = 0;
    fileQueArray[fd].ocupado = 1;
    return 0;
}

int fs_writeFile(int fd, char *buff, int size)
{
    int sizeOfBlock = getSOB();
    if (ceil(((float)fileQueArray[fd].currentLocationInFile +
        (float)size) / (float)sizeOfBlock) > NUMBEROFENTRIESALLOWED)
    {
        printf("Maximum file size surpassed of %i.\n", NUMBEROFENTRIESALLOWED);
        return -1;
    }

    int sizeOfBuffer = getSOB();
    int bytesAvailableInBuff = sizeOfBuffer - (fileQueArray[fd].currentLocationInFile % sizeOfBuffer);
    int bytesInBuffer = sizeOfBuffer - bytesAvailableInBuff;
    int expectedBytesNeed = fileQueArray[fd].currentLocationInFile + size;
    int tempBlock = fileQueArray[fd].fileStats.st_blocks - 1;
    int startWritingAt = tempBlock;
    while (fileQueArray[fd].fileStats.st_blocks - 1 < (expectedBytesNeed / sizeOfBuffer))
    {
        fileQueArray[fd].fileStats.EntryArray[tempBlock++] = allocateFreeSpace();
        fileQueArray[fd].fileStats.st_blocks++;
    }
    if (bytesInBuffer + size == sizeOfBuffer) 
    {
        memcpy(fileQueArray[fd].buff + bytesInBuffer, buff, size);
        LBAwrite(fileQueArray[fd].buff, 1, fileQueArray[fd].fileStats.EntryArray[(int)(expectedBytesNeed / sizeOfBuffer) - 1]);
        fileQueArray[fd].fileStats.st_size += sizeOfBuffer;
        fileQueArray[fd].fileDir.d_reclen += sizeOfBuffer;
        fileQueArray[fd].buffIndex = 0;
    }
    else if (bytesInBuffer + size > sizeOfBuffer) 
    {
        int bytesWrittenSoFar = 0;

        int bytesToGetfromCaller = sizeOfBuffer - bytesInBuffer;
        int bytesRemainingInCaller = size - bytesToGetfromCaller;
        bytesWrittenSoFar = bytesToGetfromCaller;
        memcpy(fileQueArray[fd].buff + bytesInBuffer, buff, bytesToGetfromCaller);
        LBAwrite(fileQueArray[fd].buff, 1, fileQueArray[fd].fileStats.EntryArray[startWritingAt]);
        fileQueArray[fd].fileStats.st_size += sizeOfBuffer;
        fileQueArray[fd].fileDir.d_reclen += sizeOfBuffer;
        int blocksWritten = 0;
        while (size - bytesWrittenSoFar > getSOB())
        {
            LBAwrite(buff + bytesWrittenSoFar, 1, fileQueArray[fd].fileStats.EntryArray[startWritingAt + blocksWritten]);
            bytesWrittenSoFar += getSOB();
            blocksWritten++;
        }

        int remainingBytes = size - bytesWrittenSoFar;
        memcpy(fileQueArray[fd].buff, buff + bytesWrittenSoFar, remainingBytes);
        fileQueArray[fd].buffIndex = remainingBytes;
    }
    else
    {
        int LBAToWriteTo = (int)ceil((float)(fileQueArray[fd].currentLocationInFile + size)/ getSOB())-1;
        if (fileQueArray[fd].fileStats.EntryArray[LBAToWriteTo] == 0)
        {
            fileQueArray[fd].fileStats.EntryArray[LBAToWriteTo] = allocateFreeSpace();
            fileQueArray[fd].fileStats.st_blocks++;
        }
        memcpy(fileQueArray[fd].buff + bytesInBuffer, buff, size);
        fileQueArray[fd].buffIndex += size;
    }

    fileQueArray[fd].fileStats.st_modtime = time(NULL);
    fileQueArray[fd].currentLocationInFile += size;

    return size;
}

int fs_readFile(int fd, char *buff, int size)
{
    int EntryArrayToBeginRead;
    int callerBufferLocation = 0;
    if (fileQueArray[fd].currentLocationInFile < fileQueArray[fd].fileStats.st_size)
    {
        EntryArrayToBeginRead = fileQueArray[fd].currentLocationInFile / getSOB(); 
        float iLimit = (float)size / (float)getSOB();
        int iLimitCast;
        if (iLimit - (int) iLimit == 0) 
        {
            iLimitCast = (int) iLimit;
        }
        else
        {
            iLimitCast = (int) iLimit + 1;
        }
        for (int i = 0; i < iLimitCast; i++) 
        {
            LBAread(fileQueArray[fd].buff, 1, fileQueArray[fd].fileStats.EntryArray[EntryArrayToBeginRead]);
            int blockOffset = (fileQueArray[fd].currentLocationInFile % getSOB());
            int bytesToWrite = getSOB() - blockOffset;
            if (fileQueArray[fd].fileStats.st_size - fileQueArray[fd].currentLocationInFile < blockOffset) 
            {
                bytesToWrite = fileQueArray[fd].fileStats.st_size - fileQueArray[fd].currentLocationInFile;
            }

            memcpy(buff + callerBufferLocation,
                fileQueArray[fd].buff + blockOffset, bytesToWrite);
            if (fileQueArray[fd].currentLocationInFile + getSOB() - blockOffset >
                fileQueArray[fd].fileStats.st_size)
            {
                callerBufferLocation = fileQueArray[fd].fileStats.st_size - (getSOB() * (fileQueArray[fd].currentLocationInFile / getSOB()));
                fileQueArray[fd].currentLocationInFile = fileQueArray[fd].fileStats.st_size;
                break;
            }
            else
            {
                callerBufferLocation += getSOB() - blockOffset;
                fileQueArray[fd].currentLocationInFile += callerBufferLocation;
            }
        }
    }

    if (callerBufferLocation > size)
    {
        printf("something went wrong in fs_readFile...\n");
        return -1;
    }
    return callerBufferLocation;
}

int fs_lSeek(int fd, int locationInBytes)
{
    printf("fs_lseek Not implemented at this time.\n");
    return 0;
}

int fs_moveDirEntry(char *name, char *path)
{
    int nameLocation = -1;
    int pathLocation = -1;
    for (int i = 1; i < NUMBEROFENTRIESALLOWED; i++)
    {
        if (strcmp(name, currentDir[i].name) == 0)
        {
            nameLocation = i;
        }
        if (strcmp(path, currentDir[i].name) == 0)
        {
            pathLocation = i;
        }
    }
    if (nameLocation == -1)
    {
        printf("Couldn't find file... good luck!\n");
        return 1;
    }
    if (pathLocation == -1 || currentDir[pathLocation].fileType != DIR)
    {
        printf("Couldnt find Directory... good luck!\n");
        return 1;
    }

    printf("Move entry is not working at this time...\n");
    return 1;

    struct fdDir tempDirEntry;
    strcpy(tempDirEntry.name, currentDir[nameLocation].name);

    fs_setcwd(path);
    return 0; // default
}
