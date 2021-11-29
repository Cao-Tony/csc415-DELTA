#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include "fsLow.h"
#include "freeSpace.h"
#include "volumeControlBlock.h"

int writeFSM();

int initializeFSM()
{
    int blockSize = getSOB();
    int totalBlocks = getTBC();
    unsigned int numberOfBytesNeededForFSM = ceil((double)totalBlocks / 4);
    int numberOfBlocksNeededForFSM = ceil((double)numberOfBytesNeededForFSM / blockSize);
    int numberOfUsedBlocks = (numberOfBlocksNeededForFSM + 1 /*VCM*/);
    volatileFSM = malloc(sizeof(_Bool) * (numberOfBytesNeededForFSM * 4));
    for (int i = 0; i < numberOfUsedBlocks; i++)
        volatileFSM[i] = 0;
    for (int i = numberOfUsedBlocks; i < (numberOfBytesNeededForFSM * 4); i++)
    {
        if (i >= totalBlocks)
            volatileFSM[i] = 0;
        else
            volatileFSM[i] = 1;
    }
    writeFSM();
    changeFSC(-1 * numberOfBlocksNeededForFSM);
    return 0;
}

int ceil4(int num)
{
    if (num % 4 != 0)
        return (num + (4 - (num % 4)));
    else
        return num;
}

char *FindFSMHex(int num)
{
    int tempNum = ceil4(num + 1) - (num + 1);
    char *temp = malloc(sizeof(char));
    switch (tempNum)
    {
    case 1:
        sprintf(temp, "%d", 7); 
        break;
    case 2:
        sprintf(temp, "%d", 3); 
        break;
    case 3:
        sprintf(temp, "%d", 1); 
        break;
    default:
        sprintf(temp, "%s", "F"); 
        break;
    }
    return temp;
}

_Bool *hexCharToBoolArray(char hexValue)
{
    char tmpValue = hexValue;
    _Bool *rArray = malloc(4 * sizeof(_Bool));
    for (int i = 0; i < 4; i++)
        rArray[i] = 0;
    if (tmpValue == '1' ||
        tmpValue == '3' ||
        tmpValue == '5' ||
        tmpValue == '7' ||
        tmpValue == '9' ||
        tmpValue == 'B' ||
        tmpValue == 'b' ||
        tmpValue == 'D' ||
        tmpValue == 'd' ||
        tmpValue == 'F' ||
        tmpValue == 'f')
        rArray[3] = 1;
    if (tmpValue == '2' ||
        tmpValue == '3' ||
        tmpValue == '6' ||
        tmpValue == '7' ||
        tmpValue == 'A' ||
        tmpValue == 'a' ||
        tmpValue == 'B' ||
        tmpValue == 'b' ||
        tmpValue == 'E' ||
        tmpValue == 'e' ||
        tmpValue == 'F' ||
        tmpValue == 'f')
        rArray[2] = 1;
    if (tmpValue == '4' ||
        tmpValue == '5' ||
        tmpValue == '6' ||
        tmpValue == '7' ||
        tmpValue == 'C' ||
        tmpValue == 'c' ||
        tmpValue == 'D' ||
        tmpValue == 'd' ||
        tmpValue == 'E' ||
        tmpValue == 'e' ||
        tmpValue == 'F' ||
        tmpValue == 'f')
        rArray[1] = 1;
    if (tmpValue == '8' ||
        tmpValue == '9' ||
        tmpValue == 'A' ||
        tmpValue == 'a' ||
        tmpValue == 'B' ||
        tmpValue == 'b' ||
        tmpValue == 'C' ||
        tmpValue == 'c' ||
        tmpValue == 'D' ||
        tmpValue == 'd' ||
        tmpValue == 'E' ||
        tmpValue == 'e' ||
        tmpValue == 'F' ||
        tmpValue == 'f')
        rArray[0] = 1;

    return rArray;
}

unsigned int boolArrayToInt(_Bool *bArray)
{
    int rVal = 0;
    for (int i = 3, j = 0; i >= 0; i--, j++)
        rVal += (bArray[j]) ? (int)pow(2, i) : 0;
    return rVal;
}

char intToHex(int hexAsInt)
{
    char rVal;
    switch (hexAsInt)
    {
    case 10:
        rVal = 'A';
        break;
    case 11:
        rVal = 'B';
        break;
    case 12:
        rVal = 'C';
        break;
    case 13:
        rVal = 'D';
        break;
    case 14:
        rVal = 'E';
        break;
    case 15:
        rVal = 'F';
        break;
    default:
        rVal = hexAsInt + '0';
        break;
    }

    return rVal;
}

int writeFSM()
{
    int blockSize = getSOB();
    int totalBlocks = getTBC();
    unsigned int numberOfBytesNeededForFSM = ceil((double)getTBC() / 4);
    int numberOfBlocksNeededForFSM = ceil((double)numberOfBytesNeededForFSM / blockSize);
    char *tempChar = malloc(sizeof(char));
    char *tempBuf = malloc(blockSize);
    _Bool *tempBool = malloc(4 * sizeof(_Bool));
    for (int fsmBlock = 1, block = 0; fsmBlock <= numberOfBlocksNeededForFSM; fsmBlock++)
    {
        for (int j = 0; j < blockSize; j++)
        {
            if (block < totalBlocks)
            {
                for (int j = 0; j < 4; j++, block++)
                    tempBool[j] = volatileFSM[block];
                sprintf(tempChar, "%c", intToHex(boolArrayToInt(tempBool)));
                memcpy(tempBuf + j, tempChar, 1);
            }
            else
                memcpy(tempBuf + j, "", 1);
        }
        LBAwrite(tempBuf, 1, fsmBlock);
    }
    free(tempBool);
    tempBool = NULL;
    free(tempChar);
    tempChar = NULL;
    free(tempBuf);
    tempBuf = NULL;
    return 0;
}

int closeFSM()
{
    writeFSM();
    free(volatileFSM);
    volatileFSM = NULL;
    return 0;
}

int openFSM()
{
    int blockSize = getSOB();
    unsigned int numberOfBytesUsedByFSM = ceil((double)getTBC() / 4);
    int numberOfBlocksUsedByFSM = ceil((double)numberOfBytesUsedByFSM / blockSize);

    char *buf = malloc(blockSize * numberOfBlocksUsedByFSM); 
    LBAread(buf, numberOfBlocksUsedByFSM, 1);

    volatileFSM = malloc(sizeof(_Bool) * (numberOfBytesUsedByFSM * 4)); 

    _Bool *FSMChunk = malloc(4 * sizeof(_Bool));

    for (int i = 0, curChunk = 0; i < (numberOfBytesUsedByFSM); i++)
    {
        char tempChar = buf[i];
        FSMChunk = hexCharToBoolArray(tempChar);

        for (int j = 0; j < 4; j++)
        {
            _Bool tmpBool = FSMChunk[j];
            volatileFSM[(curChunk * 4) + j] = tmpBool;
        }
        curChunk += 1;
    }
    free(FSMChunk);
    FSMChunk = NULL;
    free(buf);
    buf = NULL;

    return 0;
}

void printFSM()
{
    printf("Printing FSB...\n");
    unsigned int numberOfBlocksUsedInFSM = ceil((double)getTBC() / 4) * 4;
    for (int i = 0; i < numberOfBlocksUsedInFSM; i++)
    {
        if (i % 10 == 0)
            printf("\n");
        printf("%d, ", volatileFSM[i]);
    }
}

unsigned int allocateFreeSpace()
{
    unsigned int i = 0;
    while (volatileFSM[i] == 0)
        i++;
    volatileFSM[i] = 0;
    // printf("Allocating space %d\n", i);
    changeFSC(-1);
    return i;
}

int releaseFreeSpace(unsigned int lba)
{
    if (lba == 0)
    {
        printf("CRITICAL FAILURE!!\nAttempting to remove VCB!\n");
        exit(-1);
    }
    int i = volatileFSM[lba];
    // printf("releasing space %d\n", lba);
    switch (i)
    {
    case 0:
        volatileFSM[lba] = 1;
        changeFSC(1);
        return 0;
    case 1:
        return 1;
    default:
        return -1;
    }
}
