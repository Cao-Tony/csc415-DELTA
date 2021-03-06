/**************************************************************
* Class:  CSC-415 
* Name: Tony Cao, Dominique Dutton, Kandace Bishop
* Student ID: 920171613 (Tony), 920820781 (Dominique), 918762889 (Kandace)
* GitHub Handle:  Cao-Tony, kbishop1-sfsu
* Project: File System
*
* File: FreeSpace.h
*
* Description: FreeSpace allocation and managment interface
**************************************************************/
#ifndef _FREESPACE_H
#define _FREESPACE_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int initializeFSM();

int closeFSM();

int ceil4(int num);

char *FindFSMHex(int num);

_Bool *hexCharToBoolArray(char hexValue);

unsigned int boolArrayToInt(_Bool *bArray);

char intToHex(int hexAsInt);

int openFSM();

void printFSM();

unsigned int allocateFreeSpace();

int releaseFreeSpace(unsigned int);

#endif
