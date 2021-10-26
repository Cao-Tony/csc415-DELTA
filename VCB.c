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
    // writeVCB();
    return 0;
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
