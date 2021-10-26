#include <stdio.h>
#include <stdlib.h>

typedef struct fsDir {
    char name[40]; 
    int size; 
    int file_type;
    unsigned short d_reclen;
    unsigned short dir_file_position;
    uint64_t dir_starting_location;
} fsDir