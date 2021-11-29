/**************************************************************
* Class:  CSC-415 
* Name: Tony Cao, Dominique Dutton
* Student ID: 920171613 (Tony), 920820781 (Dominique)
* GitHub Handle:  Cao-Tony
* Project: File System
*
* File: VCB.h
*
* Description: volume control block management
**************************************************************/
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
