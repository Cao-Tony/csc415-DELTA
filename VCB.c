/**************************************************************
* Class:  CSC-415 
* Name: Tony Cao
* Student ID: 920171613
* GitHub Handle:  Cao-Tony
* Project: File System
*
* File: VCB.c
*
* Description: volume control block management
**************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include "fsLow.h"
#include "VCB.h"

#define MAGIC_NUMBER_INT 1141592653

typedef struct volumeControlBlock
{
    unsigned int magicNumber;
    unsigned int sizeOfBlock;
    unsigned int numOfTotalBlocks;
    unsigned int numOfFreeBlocks;
    unsigned int rootDirectory;
} volumeControlBlock;

struct volumeControlBlock *volitileVCB;

unsigned int copyForVCB(char *buffer, unsigned int value, unsigned int totalOffset);
void writeVCB();

int initializeVCB(uint64_t volumeSize, uint64_t blockSize)
{
    int totalOffset;
    char *tempVCB = malloc(blockSize);
    /****************************************************
	*  new Volume Control Block 
	****************************************************/
    volitileVCB = malloc(sizeof(struct volumeControlBlock));
    volitileVCB->magicNumber = MAGIC_NUMBER_INT;
    volitileVCB->sizeOfBlock = blockSize;
    volitileVCB->numOfTotalBlocks =ceil(volumeSize / blockSize);
    volitileVCB->numOfFreeBlocks = volumeSize / blockSize - 1;
    volitileVCB->rootDirectory = 0;
    writeVCB();
    return 0;
}

unsigned int copyForVCB(char *buffer, unsigned int value, unsigned int totalOffset)
{
    unsigned int offset, rValue;
    char result[50]; 
    rValue = totalOffset;
    sprintf(result, "%d", value);
    offset = getPlaceValue(value);
    memcpy(buffer + rValue, result, offset);
    rValue += offset;
    memcpy(buffer + rValue++, "`", 1);
    return rValue;
}

int getPlaceValue(unsigned int number)
{
    if (number == 0) return 1;
    unsigned int k = number;
    int placeValue = 0;
    while (k != 0)
    {
        k = k / 10;
        placeValue++;
    }
    return placeValue;
}

void setRootDirectory(unsigned int location)
{   
    volitileVCB->rootDirectory = location;
}

unsigned int getRootDirectory()
{
    return volitileVCB->rootDirectory;
}

void writeVCB()
{
    char *buff = malloc(volitileVCB->sizeOfBlock);
    int totalOffset = copyForVCB(buff, MAGIC_NUMBER_INT, 0);
    totalOffset = copyForVCB(buff, volitileVCB->sizeOfBlock, totalOffset);
    totalOffset = copyForVCB(buff, volitileVCB->numOfTotalBlocks, totalOffset);
    totalOffset = copyForVCB(buff, volitileVCB->numOfFreeBlocks, totalOffset);
    totalOffset = copyForVCB(buff, volitileVCB->rootDirectory, totalOffset);

    for (int i = totalOffset; i < volitileVCB->sizeOfBlock; i++)
        memcpy(buff + i, "", 1);

    LBAwrite(buff, 1, 0);
    free(buff);
    buff = NULL;
}

void closeVCB()
{
    writeVCB();
    free(volitileVCB);
    volitileVCB = NULL;
}

int openVCB(int blockSize)
{
    char *buf = malloc(blockSize); 
    volitileVCB = malloc(sizeof(struct volumeControlBlock)); 
    LBAread(buf, 1, 0);

    unsigned int extractedData = 0;
    int intMiltiuplier = 1;
    int delimCheck = 0;
    int tempInt = 1;

    int rev = 0;
    int remainder;

    for (int i = 0; i < blockSize; i++)
    {
        if (buf[i] != 96) 
        {
            tempInt = 1;
            for (int i = 1; i < intMiltiuplier; i++)
            {
                tempInt = tempInt * 10;
            }
            extractedData = extractedData + (tempInt * (buf[i] - 48));
            intMiltiuplier += 1;
        }
        else 
        {
            while (extractedData != 0) 
            {
                remainder = extractedData % 10;
                rev = rev * 10 + remainder;
                extractedData /= 10;
            }
            extractedData = rev;
            rev = 0;
            switch (delimCheck)
            {
            case 0:
                volitileVCB->magicNumber = extractedData; 
                delimCheck += 1;                   
                extractedData = 0;
                intMiltiuplier = 1;
                break;
            case 1:
                volitileVCB->sizeOfBlock = extractedData;
                delimCheck += 1;
                extractedData = 0;
                intMiltiuplier = 1;
                break;
            case 2:
                volitileVCB->numOfTotalBlocks = extractedData;
                delimCheck += 1;
                extractedData = 0;
                intMiltiuplier = 1;
                break;
            case 3:
                volitileVCB->numOfFreeBlocks = extractedData;
                delimCheck += 1;
                extractedData = 0;
                intMiltiuplier = 1;
                break;
            case 4:
                volitileVCB->rootDirectory = extractedData;
                extractedData = 0;
                intMiltiuplier = 1;
                free(buf);
                buf = NULL;
                return 0;
            }
        }
    }
    return 0;
}

int changeFSC(int blocks)
{
    if (volitileVCB->numOfFreeBlocks + blocks > 0 &&
        volitileVCB->numOfFreeBlocks + blocks < volitileVCB->numOfTotalBlocks)
    {
        volitileVCB->numOfFreeBlocks += blocks;
        return 0;
    }
    return 1;
}

unsigned int getFSC()
{
    return volitileVCB->numOfFreeBlocks;
}

unsigned int getTBC()
{
    return volitileVCB->numOfTotalBlocks;
}

unsigned int getSOB()
{
    return volitileVCB->sizeOfBlock;
}

void printVCB()
{
    printf("Printing VCB...\n");
    printf("Magic Number: %u\n", volitileVCB->magicNumber);
    printf("Size of Block: %u\n", volitileVCB->sizeOfBlock);
    printf("Number of Total Blocks: %u\n", volitileVCB->numOfTotalBlocks);
    printf("Number of Free Blocks: %u\n", volitileVCB->numOfFreeBlocks);
    printf("Root Directory: %u\n", volitileVCB->rootDirectory);
}
