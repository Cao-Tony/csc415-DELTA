#include "FreeSpace.h"

int * bitmap_ptr;

int initializeFreeSpace(){
	bitmap_ptr = malloc(total_num_blocks * size_of_block);
	int bitmap [total_num_blocks];
	for (int i = 0; i < total_num_blocks; i++){
		if (i >= 0 && i <= 5){
			bitmap[i] = 0;
		} else {
			bitmap[i] = 1;
		}
	bitmap_ptr = bitmap;
	LBAwrite(bitmap_ptr, total_num_blocks, free_space_starting_block);
	
	return ();
}
