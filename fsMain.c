#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "fsLow.h"
#include "mfs.h"
#include "volumeControlBlock.h"
#include "freeSpace.h"

// mod(pi,2)
#define MAGIC_NUMBER_CHAR "1141592653"

int fsBoot(int argc, char *argv[])
{
    char *filename;
    uint64_t volumeSize;
    uint64_t blockSize;
    int retVal;

    if (argc > 3)
    {
        filename = argv[1];
        volumeSize = atoll(argv[2]);
        blockSize = atoll(argv[3]);
    }
    else
    {
        printf("Usage: fsLowDriver volumeFileName volumeSize blockSize\n");
        return -1;
    }

    retVal = startPartitionSystem(filename, &volumeSize, &blockSize);
    printf("Opened %s, Volume Size: %llu;  BlockSize: %llu; Return %d\n", filename, (ull_t)volumeSize, (ull_t)blockSize, retVal);

    /************************************************************
	*  Volume Control Block, Free Space Map, and Root Directory
	*************************************************************/

    char *buf = malloc(blockSize * 2);
    memset(buf, 0, blockSize * 2);
    char magicNumberFromFile[10];
    char vcbBlockSize[10];
    int vcbBS = 0;
    
    int vcbErrorCode;
    int fsmErrorCode;
    int dirErrorCode;

    if (retVal == 0)
    {

        LBAread(buf, 1, 0); 

        strncpy(magicNumberFromFile, buf, 10);
        // printf("%s\n", magicNumberFromFile);
        if (strcmp(magicNumberFromFile, MAGIC_NUMBER_CHAR) != 0) 
        {
            vcbErrorCode = initializeVCB(volumeSize, blockSize);
            fsmErrorCode = initializeFSM();
            dirErrorCode = fs_initRoot();
        }
        else
        {
            // printf("Found magic number.\n");
            strncpy(vcbBlockSize, buf + 11, 10);
            for (int i = 0; i < 10; i++)
            {
                if ((int)(vcbBlockSize[i] - '0') > 10 || (int)(vcbBlockSize[i] - '0') <= 0)
                    break;
                int tmp = vcbBlockSize[i] - '0';
                vcbBS = (vcbBS * 10) + tmp;
            }
            // printf("Found Block Size.\n");
            vcbErrorCode = openVCB(vcbBS);

            fsmErrorCode = openFSM();
        }
        int loadRootEror = fs_loadRoot();
    }
    
    free(buf);
    return 0;
}

int fsShutDown()
{
    mfs_shutdown();
    closeFSM();
    closeVCB();
    closePartitionSystem();
    return 0;
}
