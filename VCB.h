#ifndef _VOLUMECONTROLBLOCK_H
#define _VOLUMECONTROLBLOCK_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int initializeVCB(u_int64_t volumeSize, u_int64_t blockSize);

int getPlaceValue(unsigned int number);

int openVCB(int blockSize);

void closeVCB();

int changeFSC(int blocks);

unsigned int getFSC();

unsigned int getTBC();

unsigned int getSOB();

void printVCB();

void setRootDirectory(unsigned int);

unsigned int getRootDirectory();

#endif