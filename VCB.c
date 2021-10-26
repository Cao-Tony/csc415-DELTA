/**************************************************************
* Class:  CSC-415 01 01 02 Spring 2021
* Name: Lake Jasper, Justin Lam, Kami Sawekchim
* Student ID: 920150605, 921097655, 918649594
* GitHub UserID:  TheJaspinater
* Project: Basic File System
*
* File: volumeControlBlock.c
*
* Description: volume control block management
*
**************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include "VCB.h"

#define MAGIC_NUMBER_INT 1141592653

typedef struct VCB {
    unsigned int block_signature;
    unsigned int size_of_block;
    unsigned int total_num_blocks;
    unsigned int num_free_blocks;
    unsigned int free_space_starting_block;
    unsigned int root_dir;
} VCB;

struct VCB * VCB;

unsigned int copyForVCB(char *buffer, unsigned int value, unsigned int totalOffset);
void writeVCB();

/**
 * @param volumeSize The volume of space allocated for the memory
 * @param blockSize The size of each chunk of block of data
 * @return error or success
 */
int initializeVCB(u_int64_t volume_size, u_int64_t block_size)
{
    int totalOffset;
    char * temp = malloc(block_size);

    // new VCB
    VCB = malloc(sizeof(struct VCB));
    VCB->block_signature = MAGIC_NUMBER_INT; 
    VCB->size_of_block = block_size;
    VCB->total_num_blocks = ceil(volume_size/block_size);
    VCB->num_free_blocks = volume_size/block_size - 1;
    VCB->root_dir = 0;
    writeVCB();
    return 0;
}

/**
 * @param buffer Takes the buffer to memcpy to
 * @param value that needs to be copied
 * @param totalOffset where to start copying at.
 * @return int New offset
 */
unsigned int copyForVCB(char *buffer, unsigned int value, unsigned int totalOffset)
{
    unsigned int offset, rValue;
    char result[50]; //Used to cast int to string
    rValue = totalOffset;
    sprintf(result, "%d", value);
    offset = getPlaceValue(value);
    memcpy(buffer + rValue, result, offset);
    rValue += offset;
    memcpy(buffer + rValue++, "`", 1);
    return rValue;
}

/** 
 * Returns how much space the value passed in takes as a string
 * (i.e. 875,321 returns 6 & 321 returns 3)
 * @param number whole number
 * @return Number of place value
 */
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

// rewrite VCB with new values stored in vcb
void writeVCB()
{
    char *buff = malloc(VCB->size_of_block);
    int totalOffset = copyForVCB(buff, MAGIC_NUMBER_INT, 0);
    totalOffset = copyForVCB(buff, VCB->size_of_block, totalOffset);
    totalOffset = copyForVCB(buff, VCB->num_free_blocks, totalOffset);
    totalOffset = copyForVCB(buff, VCB->num_free_blocks, totalOffset);
    totalOffset = copyForVCB(buff, VCB->root_dir, totalOffset);

    for (int i = totalOffset; i < VCB->size_of_block; i++)
        memcpy(buff + i, "", 1);

    LBAwrite(buff, 1, 0);
    free(buff);
    buff = NULL;
}

void closeVCB()
{
    writeVCB();
    free(VCB);
    VCB = NULL;
}

/**
 * returns 0 for success 1 for failure. LBAreads the vcb content and
 * store the data into the struct.
 */
int openVCB(int blockSize)
{
    // Get raw data from volume, store in active memory
    // TODO: Need to account for block sizes greater then 512
    char *buf = malloc(blockSize); 
    // Freed in claseVCB
    volitileVCB = malloc(sizeof(struct volumeControlBlock)); 
    // buffer to store data, number of blocks to read, starting block
    LBAread(buf, 1, 0);

    // extract data from memory and store in stuct for use and manipulation
    // final location of data before write to struct
    unsigned int extractedData = 0;
    // track current decimal place for incoming values
    int intMiltiuplier = 1;
    // track number of VCB elements extracted from buffer
    int delimCheck = 0;
    int tempInt = 1;

    //reverse unsigned int
    int rev = 0;
    int remainder;

    for (int i = 0; i < blockSize; i++)
    {
        if (buf[i] != 96) //build data until P delimiter
        {
            tempInt = 1;
            for (int i = 1; i < intMiltiuplier; i++)
            {
                tempInt = tempInt * 10;
            }
            extractedData = extractedData + (tempInt * (buf[i] - 48));
            intMiltiuplier += 1;
        }
        else //write data to volitileVCB struct
        {
            while (extractedData != 0) // reverse extracted data
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
                volitileVCB->magicNumber = extractedData; //string to int
                delimCheck += 1;                    // advance delim check
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

/**
 * Change VCB Free Block Count
 * @param blocks +/- value of blocks to modify free block counts
 * @return 1 or 0 for failure or success
 */
int changeFSC(int blocks)
{
    if (VCB->num_free_blocks + blocks > 0 &&
        VCB->num_free_blocks + blocks < VCB->total_num_blocks)
    {
        VCB->num_free_blocks += blocks;
        return 0;
    }
    return 1;
}

// get free space count
unsigned int getFSC()
{
    return VCB->num_free_blocks;
}

/**
 * Get total number of blocks
 * @return number of total blocks
 */
unsigned int getTBC()
{
    return VCB->total_num_blocks;
}

/**
 * get size of block
 * @return size of block
 */
unsigned int getSOB()
{
    return VCB->size_of_block;
}

void printVCB()
{
    printf("Printing VCB...\n");
    printf("Magic Number: %u\n", VCB->block_signature);
    printf("Size of Block: %u\n", VCB->size_of_block);
    printf("Number of Total Blocks: %u\n", VCB->total_num_blocks);
    printf("Number of Free Blocks: %u\n", VCB->num_free_blocks);
    printf("Starting of Free Space: %u\n", VCB->free_space_starting_block);
    printf("Root Directory: %u\n", VCB->root_dir);
}
