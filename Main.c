/**************************************************************
* Class:  CSC-415 
* Name: Tony Cao, Dominique Dutton, Kandace Bishop
* Student ID: 920171613 (Tony), 920820781 (Dominique), 918762889 (Kandace)
* GitHub Handle:  Cao-Tony, kbishop1-sfsu
* Project: File System
*
* File: Main.c
*
* Description: M1 demo
**************************************************************/
#include "fsMain.h"

// mod(pi,2)
#define MAGIC_NUMBER_CHAR "1141592653"

int main(int argc, char *argv[])
{
	fsBoot(argc, argv);
	fsShutDown();
	return 0;
}
