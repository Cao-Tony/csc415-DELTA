#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
//#include <fcntl.h>
#include "FreeSpace.h"
#include "VCB.h"

int * bitmap_ptr;

int initializeFreeSpace(VCB * vcb_ptr){
	int num_of_blocks = vcb_ptr->total_num_blocks;
	int block_size = vcb_ptr->size_of_block;
	bitmap_ptr = malloc(block_size * num_of_blocks);
	int bitmap [num_of_blocks];
	for (int i = 0; i < num_of_blocks; i++){
		if (i >= 0 && i <= 5){
			bitmap[i] = 0;
		} else {
			bitmap[i] = 1;
		}
	bitmap_ptr = bitmap;
	//LBAwrite(bitmap_ptr, num_of_blocks, vcb_ptr->free_space_starting_block);
	LBAwrite(bitmap_ptr, 1, vcb_ptr->free_space_starting_block);
	
	return (0);
}
