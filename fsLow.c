/**************************************************************
* Class:  CSC-415
* Name: Professor Bierman
* Student ID: N/A
* Project: File System
*
* File: fsLow.c
*
* Description: This file provides the ability to read and write 
*   Logical Blocks and is the main interface for the file system
*   project.  
*   Note that the layer uses one more block than that presented
*   to the file organization module.  This block is used to 
*   hold the partition information and is not accessible from
*   any other layer.  But, when you use the hexdump utility
*   you will see that the first block is not part of the volume.
*
*   The file created by this layer represents the physical hard 
*	drive.  It presents to the logical layer (your layer) as just
*	a logical block array (a series of blocks - nominally 512 bytes,
*	that the logical layer can utilize). 
*
*	It is imperative that the logical layer (your layer) first
*	call startPartitionSystem before using any function and when
*	finished calls closePartitionSystem() to ensure that the 
*	file that represents the physical drive is properally closed.
*
**************************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <math.h>
#include "fsLow.h"

typedef struct partitionInfo {
	char 		volumePrefix[sizeof(PART_CAPTION)+2];
	uint64_t	signature;
	uint64_t	volumesize;
	uint64_t	blocksize;
	uint64_t	numberOfBlocks;
	uint64_t	signature2;
	char		volumeName[];
	} partitionInfo_t, * partitionInfo_p;
	
partitionInfo_p partInfop = NULL;

int initializePartition (int fd, uint64_t volSize, uint64_t blockSize)
	{
	ssize_t writeRet;
	partitionInfo_p buf = malloc (blockSize);
	
	if (buf == NULL)
		{
		//abort
		return (-1)
		}
	
	strcpy(buf->volumePrefix, PART_CAPTION);
	buf->signature = PART_SIGNATURE;
	buf->volumesize = volSize;
	buf->blocksize = blockSize;
	buf->numberOfBlocks = volSize / blockSize;
	buf->signature2 = PART_SIGNATURE2;
	strcpy(buf->volumeName, "Untitled\n\n");
	
	lseek(fd, 0 , SEEK_SET);
	writeRet = write(fd, buf, blockSize);
	
	if (writeRet != blockSize)
		{
		//process error
		return (-1)
		}
	
	fsync(fd);
	uint64_t blkCount = buf->numberOfBlocks;
	memset (buf, 0, blockSize);
	
	lseek (fd, volSize, SEEK_SET);
	writeRet = write(fd, buf, blockSize);		
	fsync(fd);
	printf("Created a volume with %llu bytes, broken into %llu blocks of %llu bytes.\n",
				 (ull_t)volSize, (ull_t)blkCount, (ull_t)blockSize);	
	free (buf);
	buf = NULL;
	return PART_NOERROR;
	}

int startPartitionSystem (char * filename, uint64_t * volSize, uint64_t * blockSize)
	{
	int fd;
	int retVal = PART_NOERROR;
	int accessRet = access(filename, F_OK);
	printf ("File %s does %sexist, errno = %d\n", filename, accessRet==-1?"not ":"",errno);
	
	accessRet = access(filename, R_OK | W_OK);
	printf ("File %s %sgood to go, errno = %d\n", filename, accessRet==-1?"not ":"",errno);
	
	if (accessRet == -1)
		{
		if (errno == ENOENT)  
			{
			fd = open(filename, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			if (fd == -1)
				{
				//Error
				return -1;
				}
				
			uint64_t blksz = *blockSize;
			printf("Block size is : %llu\n", (ull_t)blksz);
			if (blksz < MINBLOCKSIZE) 
				blksz = MINBLOCKSIZE;
				
			if ((blksz & (blksz - 1)) != 0) 
				{
				printf("%llu is not a power of 2\n", (ull_t)blksz);
				
				blksz = 1 << (uint64_t)(ceil(log2(blksz)));
				printf("Block size is now: %llu\n", (ull_t)blksz);
				*blockSize = blksz;
				} 

			uint64_t blockCount = *volSize / blksz;
			*volSize = blockCount * blksz;
				
			int initRet = initializePartition (fd, *volSize, *blockSize);
			close (fd);
			}
		else
			{
			printf ("About to abort - problem opening file.  Error No: %d\n", errno);
			return -1; 
			}
		}
	
	fd = open(filename, O_RDWR);
	partitionInfo_p buf = malloc (MINBLOCKSIZE);
	uint64_t readCount = read (fd, buf, MINBLOCKSIZE);
	if ((buf->signature == PART_SIGNATURE) && (buf->signature2 == PART_SIGNATURE2))
		{
		*volSize = buf->volumesize;
		*blockSize = buf->blocksize;
		partInfop = malloc (sizeof(partitionInfo_t)+strlen(buf->volumeName)+4);
		memcpy(partInfop, buf, sizeof(partitionInfo_t)+strlen(buf->volumeName)+4);
		partInfop->filename = malloc (strlen(filename)+4);
		strcpy(partInfop->filename, filename);
		partInfop->fd = fd;
		retVal = PART_NOERROR;
		}
	else
		{
		*volSize = 0;
		*blockSize = 0;
		retVal = PART_ERR_INVALID;
		}
		
	free (buf);
	if (retVal != PART_NOERROR)
		close (fd);
	return retVal;
	}

int closePartitionSystem ()
	{
	fsync(partInfop->fd);
	close (partInfop->fd);
	free (partInfop->filename);
	free (partInfop);
	
	partInfop = NULL;
	return 0;
	}
	
uint64_t LBAwrite (void * buffer, uint64_t lbaCount, uint64_t lbaPosition)
	{
	struct flock fl;
	
	if (partInfop == NULL)		
		return 0;
		
	if(lbaCount == 0)
		return 0;
	
	fl.l_type = F_WRLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = (lbaPosition * partInfop->blocksize)+ partInfop->blocksize;
	fl.l_len = lbaCount * partInfop->blocksize;
	
	if ((lbaPosition + lbaCount) > partInfop->numberOfBlocks)
		{
		if (lbaPosition+1 >= partInfop->numberOfBlocks)
			return 0;	
		
		lbaCount = 	partInfop->numberOfBlocks - lbaPosition;
		fl.l_len = lbaCount * partInfop->blocksize;
		}
				
	fcntl(partInfop->fd, F_SETLKW, &fl);

	lseek (partInfop->fd, fl.l_start, SEEK_SET);
	uint64_t retWrite = write(partInfop->fd, buffer, fl.l_len);
	
	fsync(partInfop->fd);

	fl.l_type = F_UNLCK;
	fcntl(partInfop->fd, F_SETLKW, &fl);
	
	return retWrite / partInfop->blocksize;
	}

uint64_t LBAread (void * buffer, uint64_t lbaCount, uint64_t lbaPosition) 
	{
	struct flock fl;
	
	if (partInfop == NULL)		
		return 0;
		
	if(lbaCount == 0)
		return 0;

	fl.l_type = F_RDLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = (lbaPosition * partInfop->blocksize)+ partInfop->blocksize;
	fl.l_len = lbaCount * partInfop->blocksize;

	if ((lbaPosition + lbaCount) > partInfop->numberOfBlocks)
		{
		if (lbaPosition+1 >= partInfop->numberOfBlocks)
			return 0;	
		
		lbaCount = 	partInfop->numberOfBlocks - lbaPosition;
		fl.l_len = lbaCount * partInfop->blocksize;
		}
		
	fcntl(partInfop->fd, F_SETLKW, &fl);

	lseek (partInfop->fd, fl.l_start, SEEK_SET);
	read(partInfop->fd, buffer, fl.l_len);

	fl.l_type = F_UNLCK;
	fcntl(partInfop->fd, F_SETLKW, &fl);
	
	return 0;
	}


