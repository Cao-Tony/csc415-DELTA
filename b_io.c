/**************************************************************
* Class:  CSC-415 
* Name: Tony Cao
* Student ID: 920171613
* GitHub Handle:  Cao-Tony
* Project: File System
*
* File: b_io.c
*
* Description: Main driver for file system assignment
**************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h> 
#include <string.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "b_io.h"
#include "fsLow.h"
#include "mfs.h"

#define MAXFCBS 20
#define B_CHUNK_SIZE 512

typedef struct b_fcb
{
	int linuxFd;  
	char *buf;	  
	int index;	  
	int buflen;	  
	int fileFlag; 
} b_fcb;

b_fcb fcbArray[MAXFCBS];

int startup = 0; 

void b_init()
{
	
	for (int i = 0; i < MAXFCBS; i++)
	{
		fcbArray[i].linuxFd = -1; 
	}

	startup = 1;
}


int b_getFCB()
{
	for (int i = 0; i < MAXFCBS; i++)
	{
		if (fcbArray[i].linuxFd == -1)
		{
			fcbArray[i].linuxFd = -2; 
			return i;				  
		}
	}
	return (-1); 
}

int b_open(char *filename, int flags)
{
	//printf("Calling b_open\n");
	int fd;
	int returnFd;

	if (startup == 0)
		b_init(); //Initialize our system

	fd = fs_openFile(filename, flags);
	if (fd == -1)
		return (-1); //error opening filename

	returnFd = b_getFCB();			 
									 
	fcbArray[returnFd].linuxFd = fd; 

	fcbArray[returnFd].buf = malloc(B_CHUNK_SIZE);
	if (fcbArray[returnFd].buf == NULL)
	{
		fs_closeFile(fd);					
		fcbArray[returnFd].linuxFd = -1; //Free FCB
		return -1;
	}

	fcbArray[returnFd].buflen = 0;		 
	fcbArray[returnFd].index = 0;		 
	fcbArray[returnFd].fileFlag = flags; 
	return (returnFd);					 // all set
}

int b_write(b_io_fd fd, char *buffer, int count)
{
	// printf("Calling b_write\n");
	if (startup == 0)
		b_init(); 

	if ((fd < 0) || (fd >= MAXFCBS))
	{
		return (-1); //invalid file descriptor
	}

	if (fcbArray[fd].linuxFd == -1) //File not open for this descriptor
	{
		return -1;
	}

	if (fcbArray[fd].fileFlag == O_RDONLY)
	{
		printf("File is Read only.\n");
		return 0;
	}

	if (fcbArray[fd].buflen == 0)
		fcbArray[fd].buflen = B_CHUNK_SIZE;

	int remainder;
	remainder = (B_CHUNK_SIZE - fcbArray[fd].index - count) * -1;
	int case1;
	int case2;
	int case3;

	if (remainder >= 0 && remainder < B_CHUNK_SIZE)
	{
		case1 = count - remainder;
		case2 = 0;
		case3 = remainder;
	}
	else if (remainder >= 0 && remainder > B_CHUNK_SIZE)
	{
		(fcbArray[fd].index == 0) ?
			(case1 = 0) : (case1 = B_CHUNK_SIZE - fcbArray[fd].index);
		case2 = ((count - case1) / B_CHUNK_SIZE) * B_CHUNK_SIZE;
		case3 = count - (case1 + case2);
	}
	else
	{
		case1 = 0;
		case2 = 0;
		case3 = count;
	}

	if (case1 > 0)
	{	
		memcpy(fcbArray[fd].buf + fcbArray[fd].index, buffer, case1);
		fcbArray[fd].index = 0;
		if (!fs_writeFile(fcbArray[fd].linuxFd, fcbArray[fd].buf, B_CHUNK_SIZE))
		{
			perror("Error writing\n");
			exit(1);
		}
	}

	if (case2 > 0)
	{
		if (!fs_writeFile(fcbArray[fd].linuxFd, buffer + case1, case2))
		{
			perror("Error writing\n");
			exit(1);
		}
	}

	memcpy(fcbArray[fd].buf + fcbArray[fd].index, buffer + (case1 + case2), case3);
	fcbArray[fd].index += case3;
	return case1 + case2 + case3;
}

int b_read(b_io_fd fd, char *buffer, int count)
{
	//printf("Calling b_read\n");
	int bytesRead;				 
	int bytesReturned;			 
	int part1, part2, part3;	  
	int numberOfBlocksToCopy;	 
	int remainingBytesInMyBuffer; 

	if (startup == 0)
		b_init(); //Initialize our system

	if ((fd < 0) || (fd >= MAXFCBS))
	{
		return (-1); //invalid file descriptor
	}

	if (fcbArray[fd].linuxFd == -1) //File not open for this descriptor
	{
		return -1;
	}

	remainingBytesInMyBuffer = fcbArray[fd].buflen - fcbArray[fd].index;

	if (remainingBytesInMyBuffer >= count) 
	{
		part1 = count; 
		part2 = 0;
		part3 = 0; 
	else
	{
		part1 = remainingBytesInMyBuffer; 
		part3 = count - remainingBytesInMyBuffer; 

		numberOfBlocksToCopy = part3 / B_CHUNK_SIZE; 
		part2 = numberOfBlocksToCopy * B_CHUNK_SIZE;
		part3 = part3 - part2; 
	}

	if (part1 > 0) 
	{
		memcpy(buffer, fcbArray[fd].buf + fcbArray[fd].index, part1);
		fcbArray[fd].index = fcbArray[fd].index + part1;
	}

	if (part2 > 0) 
	{
		bytesRead = fs_readFile(fcbArray[fd].linuxFd, buffer + part1, numberOfBlocksToCopy * B_CHUNK_SIZE);
		part2 = bytesRead; 
	}

	if (part3 > 0) 
	{
		bytesRead = fs_readFile(fcbArray[fd].linuxFd, fcbArray[fd].buf, B_CHUNK_SIZE);

		fcbArray[fd].index = 0;
		fcbArray[fd].buflen = bytesRead; 

		if (bytesRead < part3) 
			part3 = bytesRead;

		if (part3 > 0) 
		{
			memcpy(buffer + part1 + part2, fcbArray[fd].buf + fcbArray[fd].index, part3);
			fcbArray[fd].index = fcbArray[fd].index + part3; 
		}
	}
	bytesReturned = part1 + part2 + part3;
	return (bytesReturned);
}

void b_close(int fd)
{
	// printf("Calling b_close\n");
	if (fcbArray[fd].fileFlag != O_RDONLY && fcbArray[fd].index > 0)
	{
		if (!fs_writeFile(fcbArray[fd].linuxFd, fcbArray[fd].buf, fcbArray[fd].index))
		{
			perror("Error writing\n");
			exit(1);
		}
	}
	fs_closeFile(fcbArray[fd].linuxFd); 
	free(fcbArray[fd].buf);		 
	fcbArray[fd].buf = NULL;	 
	fcbArray[fd].linuxFd = -1;	 // return this FCB to list of available FCB's
}

int b_seek (int fd, off_t offset, int whence)
{
	printf("b_seek Not implemented at this time.\n");
	return fs_lSeek(fd, whence);
}