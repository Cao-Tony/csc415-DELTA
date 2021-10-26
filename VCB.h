#ifndef _VOLUMECONTROLBLOCK_H
#define _VOLUMECONTROLBLOCK_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int initializeVCB(u_int64_t volumeSize, u_int64_t blockSize);

void printVCB();

#endif